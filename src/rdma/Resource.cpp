#include "rdma/Rdma.h"
#include "utility/HugePageAlloc.h"
#include <arpa/inet.h>

#include <cmath>

#define likely(x) __builtin_expect(!!(x), 1)

bool createContext(RdmaContext *context, const std::string &rnic, uint8_t port,
                   int gidIndex) {

  ibv_device *dev = NULL;
  ibv_context *ctx = NULL;
  ibv_pd *pd = NULL;
  ibv_port_attr portAttr;

  int devicesNum;
  struct ibv_device **deviceList = ibv_get_device_list(&devicesNum);
  if (!deviceList) {
    Debug::notifyError("failed to get IB devices list");
    goto CreateResourcesExit;
  }

  if (!devicesNum) {
    Debug::notifyInfo("found %d device(s)", devicesNum);
    goto CreateResourcesExit;
  }

  int dev_idx;
  for (dev_idx = 0; dev_idx < devicesNum; ++dev_idx) {
    if (strcmp(ibv_get_device_name(deviceList[dev_idx]), rnic.c_str()) == 0) {
      break;
    }
  }

  if (dev_idx >= devicesNum) {
    Debug::notifyError("ib device wasn't found");
    goto CreateResourcesExit;
  }

  dev = deviceList[dev_idx];
  printf("I open %s :)\n", ibv_get_device_name(dev));

  // get device handle
  ctx = ibv_open_device(dev);
  if (!ctx) {
    Debug::notifyError("failed to open device");
    goto CreateResourcesExit;
  }
  /* We are now done with device list, free it */
  ibv_free_device_list(deviceList);
  deviceList = NULL;

  // query port properties
  if (ibv_query_port(ctx, port, &portAttr)) {
    Debug::notifyError("ibv_query_port failed");
    goto CreateResourcesExit;
  }

  // allocate Protection Domain
  // Debug::notifyInfo("Allocate Protection Domain");
  pd = ibv_alloc_pd(ctx);
  if (!pd) {
    Debug::notifyError("ibv_alloc_pd failed");
    goto CreateResourcesExit;
  }

  if (ibv_query_gid(ctx, port, gidIndex, &context->gid)) {
    Debug::notifyError("could not get gid for port: %d, gidIndex: %d", port,
                       gidIndex);
    goto CreateResourcesExit;
  }

  // Success :)
  context->devIndex = dev_idx;
  context->gidIndex = gidIndex;
  context->port = port;
  context->ctx = ctx;
  context->pd = pd;
  context->lid = portAttr.lid;

  return true;

/* Error encountered, cleanup */
CreateResourcesExit:
  Debug::notifyError("Error Encountered, Cleanup ...");

  if (pd) {
    ibv_dealloc_pd(pd);
    pd = NULL;
  }
  if (ctx) {
    ibv_close_device(ctx);
    ctx = NULL;
  }
  if (deviceList) {
    ibv_free_device_list(deviceList);
    deviceList = NULL;
  }

  return false;
}

bool destoryContext(RdmaContext *context) {
  bool rc = true;
  if (context->pd) {
    if (ibv_dealloc_pd(context->pd)) {
      Debug::notifyError("Failed to deallocate PD");
      rc = false;
    }
  }
  if (context->ctx) {
    if (ibv_close_device(context->ctx)) {
      Debug::notifyError("failed to close device context");
      rc = false;
    }
  }

  return rc;
}

ibv_mr *createMemoryRegion(uint64_t mm, uint64_t mmSize, RdmaContext *ctx,
                           bool is_physical_mapping) {

  ibv_mr *mr = NULL;

  auto flag = 0;
  mr =
      ibv_reg_mr(ctx->pd, (void *)mm, mmSize,
                 IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
                     IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_ATOMIC | flag);
  if (!mr) {
    Debug::notifyError(
        "Memory registration failed, mm=%p, mm_end=%p, mmSize=%lld", mm,
        mm + mmSize, mmSize);
  }

  return mr;
}

bool createQueuePair(ibv_qp **qp, ibv_qp_type mode, ibv_cq *send_cq,
                     ibv_cq *recv_cq, RdmaContext *context,
                     uint32_t qpsMaxDepth, ibv_srq *srq,
                     uint32_t maxInlineData) {
#ifndef OFED_VERSION_4
  struct ibv_qp_init_attr_ex attr;
  memset(&attr, 0, sizeof(attr));

  attr.qp_type = mode;
  attr.sq_sig_all = 0;
  attr.send_cq = send_cq;
  attr.recv_cq = recv_cq;
  attr.srq = srq;
  attr.pd = context->pd;

  if (mode == IBV_QPT_RC) {
    attr.comp_mask = IBV_QP_INIT_ATTR_CREATE_FLAGS | IBV_QP_INIT_ATTR_PD;
    // attr.max_atomic_arg = 32;
  } else {
    attr.comp_mask = IBV_QP_INIT_ATTR_PD;
  }

  attr.cap.max_send_wr = qpsMaxDepth;
  attr.cap.max_recv_wr = qpsMaxDepth;
  attr.cap.max_send_sge = 1;
  attr.cap.max_recv_sge = 1;
  attr.cap.max_inline_data = 0;

  *qp = ibv_create_qp_ex(context->ctx, &attr);
  if (!(*qp)) {
    Debug::notifyError("Failed to create QP");
    perror("XXX");
    return false;
  }
#else
  struct ibv_exp_qp_init_attr attr;
  memset(&attr, 0, sizeof(attr));

  attr.qp_type = mode;
  attr.sq_sig_all = 0;
  attr.send_cq = send_cq;
  attr.recv_cq = recv_cq;
  attr.srq = srq;
  attr.pd = context->pd;

  if (mode == IBV_QPT_RC) {
    attr.comp_mask = IBV_EXP_QP_INIT_ATTR_CREATE_FLAGS |
                     IBV_EXP_QP_INIT_ATTR_PD | IBV_EXP_QP_INIT_ATTR_ATOMICS_ARG;
    attr.max_atomic_arg = maxInlineData;
  } else {
    attr.comp_mask = IBV_EXP_QP_INIT_ATTR_PD;
  }

  attr.cap.max_send_wr = qpsMaxDepth;
  attr.cap.max_recv_wr = qpsMaxDepth;

  attr.cap.max_send_sge = 2;
  attr.cap.max_recv_sge = 1;
  attr.cap.max_inline_data = maxInlineData;

  *qp = ibv_exp_create_qp(context->ctx, &attr);
  if (!(*qp)) {
    Debug::notifyError("Failed to create QP");
    return false;
  }

#endif

  return true;
}

ibv_srq *createSRQ(RdmaContext *context, size_t queue_size) {
  struct ibv_srq *srq;
  struct ibv_srq_init_attr srq_init_attr;

  memset(&srq_init_attr, 0, sizeof(srq_init_attr));

  srq_init_attr.attr.max_wr = queue_size;
  srq_init_attr.attr.max_sge = 2;

  srq = ibv_create_srq(context->pd, &srq_init_attr);
  if (!srq) {
    fprintf(stderr, "Error, ibv_create_srq() failed\n");
  }

  return srq;
}

bool createQueuePair(ibv_qp **qp, ibv_qp_type mode, ibv_cq *cq,
                     RdmaContext *context, uint32_t qpsMaxDepth, ibv_srq *srq,
                     uint32_t maxInlineData) {
  return createQueuePair(qp, mode, cq, cq, context, qpsMaxDepth, srq,
                         maxInlineData);
}

void fillAhAttr(ibv_ah_attr *attr, uint32_t remoteLid, uint8_t *remoteGid,
                RdmaContext *context) {

  (void)remoteGid;

  memset(attr, 0, sizeof(ibv_ah_attr));
  attr->dlid = remoteLid;
  attr->sl = 0;
  attr->src_path_bits = 0;
  attr->port_num = context->port;

  // attr->is_global = 0;

  // fill ah_attr with GRH
  attr->is_global = 1;
  memcpy(&attr->grh.dgid, remoteGid, 16);
  attr->grh.flow_label = 0;
  attr->grh.hop_limit = HOP_LIMIT;
  attr->grh.sgid_index = context->gidIndex;
  attr->grh.traffic_class = 0;
}

#include "rdma/mlx5_defs.h"
// set is to 8 can cap the IOPS to 28Mops, since cache line contention between
// CPU cores and the CPU’s on-die PCIe controller
constexpr size_t kRecvCQDepth = 128;
ibv_srq *create_mp_srq(RdmaContext *context, uint32_t max_wr,
                       uint32_t log_num_of_strides, uint32_t log_stride_size,
                       ibv_cq **cq, bool cq_overrun) {
  struct ibv_exp_create_srq_attr srq_init_attr;
  memset(&srq_init_attr, 0, sizeof(srq_init_attr));

  srq_init_attr.base.attr.max_wr = max_wr;
  srq_init_attr.base.attr.max_sge = 1;
  srq_init_attr.srq_type = IBV_EXP_SRQT_TAG_MATCHING;
  srq_init_attr.pd = context->pd;

  srq_init_attr.comp_mask =
      IBV_EXP_CREATE_SRQ_CQ | IBV_EXP_CREATE_SRQ_TM | IBV_EXP_CREATE_SRQ_MP_WR;
  srq_init_attr.mp_wr.log_num_of_strides = log_num_of_strides;
  srq_init_attr.mp_wr.log_stride_size = log_stride_size;
  srq_init_attr.tm_cap.max_num_tags = 1;
  srq_init_attr.tm_cap.max_ops = 1;

  {
    struct ibv_exp_cq_init_attr cq_init_attr;
    memset(&cq_init_attr, 0, sizeof(cq_init_attr));
    *cq = ibv_exp_create_cq(context->ctx, kRecvCQDepth / 2, nullptr, nullptr, 0,
                            &cq_init_attr);

    if (*cq == nullptr) {
      printf("Failed to create RECV CQ\n");
    }

    if (cq_overrun) {
      // Modify the RECV CQ to ignore overrun
      struct ibv_exp_cq_attr cq_attr;
      memset(&cq_attr, 0, sizeof(cq_attr));
      cq_attr.comp_mask = IBV_EXP_CQ_ATTR_CQ_CAP_FLAGS;
      cq_attr.cq_cap_flags = IBV_EXP_CQ_IGNORE_OVERRUN;

      if (ibv_exp_modify_cq(*cq, &cq_attr, IBV_EXP_CQ_CAP_FLAGS) != 0) {
        printf("Failed to modify RECV CQ\n");
      }

      auto *_mlx5_cq = reinterpret_cast<mlx5_cq *>(*cq);

      if (kRecvCQDepth != std::pow(2, _mlx5_cq->cq_log_size)) {
        printf("mlx5 CQ depth does not match kRecvCQDepth\n");
      }
      if (_mlx5_cq->buf_a.buf == nullptr) {
        printf("mlx5 CQ a buf is null\n");
      }
      volatile mlx5_cqe64 *recv_cqe_arr =
          reinterpret_cast<volatile mlx5_cqe64 *>(_mlx5_cq->buf_a.buf);

      size_t kStridesPerWQE = (1ull << log_stride_size);
      for (size_t i = 0; i < kRecvCQDepth; i++) {
        recv_cqe_arr[i].wqe_id = htons(UINT16_MAX);

        recv_cqe_arr[i].wqe_counter =
            htons(kStridesPerWQE - (kRecvCQDepth - i));
      }
    }
  }

  srq_init_attr.cq = *cq;

  ibv_srq *srq = ibv_exp_create_srq(context->ctx, &srq_init_attr);
  if (!srq) {
    fprintf(stderr, "Error, ibv_create_srq() failed\n");
    return nullptr;
  }

  return srq;
}
