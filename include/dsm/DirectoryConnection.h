#ifndef __DIRECTORYCONNECTION_H__
#define __DIRECTORYCONNECTION_H__

#include "RCMapping.h"
#include "RawMessageConnection.h"

struct RemoteConnection;

struct DirectoryConnection {

  friend class DSMKeeper;
 
  const static size_t kMpSrqSize = 64;
  const static size_t kLogNumStrides = 10;
  const static size_t kLogStrideBytes = 6;
  const static size_t kStrideBufSize =
      (1ull << kLogNumStrides) * (1ull << kLogStrideBytes);


private:
  uint16_t dirID;

  RdmaContext ctx;
  ibv_cq *cq;
  ibv_srq *srq;

  ibv_qp **data2app[config::kMaxNetThread];

  ibv_mr *dsmMR;
  void *dsmPool;
  uint64_t dsmSize;
  uint32_t dsmLKey;

  ibv_mr *cacheMR;
  void *cachePool;
  uint64_t cacheSize;
  uint32_t cacheLKey;

  RemoteConnection *remoteInfo;

public:
  DirectoryConnection(uint16_t dirID, void *dsmPool, uint64_t dsmSize,
                      uint32_t machineNR, RemoteConnection *remoteInfo,
                      void *cachePool, uint64_t cacheSize,
                      const std::string &nic_name, RCMapping *rc_mapping);

  ibv_qp *get_qp(uint16_t node_id, uint16_t thread_id) {
    return data2app[thread_id][node_id];
  }

  void recv(char *buffer, size_t size) {
    rdmaReceive(srq, (uint64_t)buffer, size, cacheLKey);
  }

  char *get_recv_buf(size_t size) {
    assert(cacheSize >= size);
    return (char *)cachePool;
  }

  ibv_srq *get_srq() { return srq; }
};

#endif /* __DIRECTORYCONNECTION_H__ */
