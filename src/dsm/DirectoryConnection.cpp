#include "dsm/DirectoryConnection.h"
#include "dsm/Connection.h"

DirectoryConnection::DirectoryConnection(uint16_t dirID, void *dsmPool,
                                         uint64_t dsmSize, uint32_t machineNR,
                                         RemoteConnection *remoteInfo,
                                         void *cachePool, uint64_t cacheSize,
                                         const std::string &nic_name,
                                         RCMapping *rc_mapping)
    : dirID(dirID), remoteInfo(remoteInfo) {

  createContext(&ctx, nic_name);
  cq = ibv_create_cq(ctx.ctx, RPC_QUEUE_SIZE, NULL, NULL, 0);

  // dsm memory
  this->dsmPool = dsmPool;
  this->dsmSize = dsmSize;
  this->dsmMR = createMemoryRegion((uint64_t)dsmPool, dsmSize, &ctx);
  this->dsmLKey = dsmMR->lkey;

  // rdma buffer memory
  this->cachePool = cachePool;
  this->cacheSize = cacheSize;
  this->cacheMR = createMemoryRegion((uint64_t)cachePool, cacheSize, &ctx);
  this->cacheLKey = cacheMR->lkey;

  ibv_cq *srq_cq;
  this->srq =
      create_mp_srq(&ctx, kMpSrqSize, kLogNumStrides, kLogStrideBytes, &srq_cq);
  printf("create mp srq\n");

  // dirID -> [node_id, thread_id]
  for (int i = 0; i < config::kMaxNetThread; ++i) {
    data2app[i] = new ibv_qp *[machineNR];
    for (size_t k = 0; k < machineNR; ++k) {
      data2app[i][k] = nullptr;
    }
  }

  auto list = rc_mapping->get_client_thread_list(dirID);
  for (auto c : *list) {
    createQueuePair(&data2app[c.second][c.first], IBV_QPT_RC, cq, &ctx,
                    RPC_QUEUE_SIZE, srq);
  }
}
