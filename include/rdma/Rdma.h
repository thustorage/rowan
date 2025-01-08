#ifndef _RDMA_H__
#define _RDMA_H__

#define forceinline inline __attribute__((always_inline))

#include <assert.h>
#include <cstring>
#include <infiniband/verbs.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <list>
#include <string>

#include "utility/Debug.h"

#define OFED_VERSION_4

#ifndef OFED_VERSION_4
static_assert(false, "only ofed 4.* supports experimental verbs")
#endif

#define NIC_MTU_SIZE IBV_MTU_4096
#define HOP_LIMIT 1
#define kMaxInlineData 64
#define MAX_POST_LIST 32
#define UD_PKEY 0x11111111
#define PSN 3185

extern int kMaxDeviceMemorySize;

struct RdmaContext {
  uint8_t devIndex;
  uint8_t port;
  int gidIndex;

  ibv_context *ctx;
  ibv_pd *pd;

  uint16_t lid;
  union ibv_gid gid;

  RdmaContext() : ctx(NULL), pd(NULL) {}
};

//// Resource.cpp
bool createContext(RdmaContext *context, const std::string &rnic = "mlx5_0", uint8_t port = 1, int gidIndex = 3);
bool destoryContext(RdmaContext *context);

ibv_mr *createMemoryRegion(uint64_t mm, uint64_t mmSize, RdmaContext *ctx,
                           bool is_physical_mapping = false);

bool createQueuePair(ibv_qp **qp, ibv_qp_type mode, ibv_cq *cq,
                     RdmaContext *context, uint32_t qpsMaxDepth = 1024, 
                     ibv_srq *srq = nullptr,
                     uint32_t maxInlineData = kMaxInlineData);

bool createQueuePair(ibv_qp **qp, ibv_qp_type mode, ibv_cq *send_cq,
                     ibv_cq *recv_cq, RdmaContext *context,
                     uint32_t qpsMaxDepth = 1024,
                     ibv_srq *srq = nullptr,
                     uint32_t maxInlineData = kMaxInlineData);

ibv_srq *createSRQ(RdmaContext *context, size_t queue_size);
ibv_srq *create_mp_srq(RdmaContext *context, uint32_t max_wr,
                       uint32_t log_num_of_strides, uint32_t log_stride_size,
                       ibv_cq **cq, bool cq_overrun = true);

void fillAhAttr(ibv_ah_attr *attr, uint32_t remoteLid, uint8_t *remoteGid,
                RdmaContext *context);

//// StateTrans.cpp

bool modifyQPtoReset(struct ibv_qp *qp);
bool modifyQPtoInit(struct ibv_qp *qp, RdmaContext *context);
bool modifyQPtoRTR(struct ibv_qp *qp, uint32_t remoteQPN, uint16_t remoteLid,
                   uint8_t *gid, RdmaContext *context);
bool modifyQPtoRTS(struct ibv_qp *qp);

bool modifyUDtoRTS(struct ibv_qp *qp, RdmaContext *context);

//// Operation.cpp
int pollWithCQ(ibv_cq *cq, int pollNumber, struct ibv_wc *wc);
int pollOnce(ibv_cq *cq, int pollNumber, struct ibv_wc *wc);

bool rdmaSend(ibv_qp *qp, uint64_t source, uint64_t size, uint32_t lkey,
              ibv_ah *ah, uint32_t remoteQPN, bool isSignaled = false);
bool rdmaSend2Sge(ibv_qp *qp, uint64_t source1, uint64_t size1,
                  uint64_t source2, uint64_t size2, uint32_t lkey1,
                  uint32_t lkey2, ibv_ah *ah, uint32_t remoteQPN,
                  bool isSignaled);

bool rdmaSend(ibv_qp *qp, uint64_t source, uint64_t size, uint32_t lkey,
              int32_t imm = -1, bool isSignaled = false);

bool rdmaReceive(ibv_qp *qp, uint64_t source, uint64_t size, uint32_t lkey,
                 uint64_t wr_id = 0);
bool rdmaReceive(ibv_srq *srq, uint64_t source, uint64_t size, uint32_t lkey);

bool rdmaRead(ibv_qp *qp, uint64_t source, uint64_t dest, uint64_t size,
              uint32_t lkey, uint32_t remoteRKey, bool signal = true,
              uint64_t wrID = 0);
bool rdmaWrite(ibv_qp *qp, uint64_t source, uint64_t dest, uint64_t size,
               uint32_t lkey, uint32_t remoteRKey, int32_t imm = -1,
               bool isSignaled = true, uint64_t wrID = 0);

struct SourceList {
  const void *addr;
  uint16_t size;
};

bool rdmaWriteVector(ibv_qp *qp, const SourceList *source_list,
                     const int list_size, uint64_t dest, uint32_t lkey,
                     uint32_t remoteRKey, bool isSignaled = true,
                     uint64_t wrID = 0, int sgePerWr = 30);

bool rdmaFetchAndAdd(ibv_qp *qp, uint64_t source, uint64_t dest, uint64_t add,
                     uint32_t lkey, uint32_t remoteRKey, bool signal = true, uint64_t wrID = 0);

bool rdmaCompareAndSwap(ibv_qp *qp, uint64_t source, uint64_t dest,
                        uint64_t compare, uint64_t swap, uint32_t lkey,
                        uint32_t remoteRKey, bool signal = true, uint64_t wrID = 0);
                        
//// Utility.cpp
void rdmaQueryQueuePair(ibv_qp *qp);


#endif
