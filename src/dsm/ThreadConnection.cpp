#include "dsm/ThreadConnection.h"
#include "dsm/Connection.h"

ThreadConnection::ThreadConnection(uint16_t node_id, uint16_t threadID, void *cachePool,
                                   uint64_t cacheSize, uint32_t machineNR,
                                   RemoteConnection *remoteInfo,
                                   const std::string &nic_name, RCMapping *rc_mapping)
    : threadID(threadID), remoteInfo(remoteInfo) {

  createContext(&ctx, nic_name);
  cq = ibv_create_cq(ctx.ctx, RPC_QUEUE_SIZE, NULL, NULL, 0);
  ud_cq = ibv_create_cq(ctx.ctx, RPC_QUEUE_SIZE, NULL, NULL, 0);

  message = new RawMessageConnection(ctx, ud_cq, RPC_QUEUE_SIZE);

  this->cachePool = cachePool;
  cacheMR = createMemoryRegion((uint64_t)cachePool, cacheSize, &ctx);
  cacheLKey = cacheMR->lkey;

  // [node_id, threadID] -> dirID
  target_dirID = rc_mapping->get_target_dir_id(node_id, threadID);
  for (int i = 0; i < NR_DIRECTORY; ++i) {
    data[i] = new ibv_qp *[machineNR];
    for (size_t k = 0; k < machineNR; ++k) {
      if (i == target_dirID) {
        createQueuePair(&data[i][k], IBV_QPT_RC, cq, &ctx);
      } else {
        data[i][k] = nullptr;
      }
    }
  }
}

void ThreadConnection::sendMessage(RpcMessage *m, uint16_t node_id,
                                   uint16_t t_id) {

  message->sendRawMessage(m, remoteInfo[node_id].appMessageQPN[t_id],
                          remoteInfo[node_id].appAh[t_id]);
}
