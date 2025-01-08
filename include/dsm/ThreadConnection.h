#ifndef __THREADCONNECTION_H__
#define __THREADCONNECTION_H__

#include "RCMapping.h"
#include "RawMessageConnection.h"

struct RemoteConnection;

// app thread
struct ThreadConnection {

  uint16_t threadID;
  uint16_t target_dirID;

  RdmaContext ctx;
  ibv_cq *cq;
  ibv_cq *ud_cq;

  RawMessageConnection *message;

  ibv_qp **data[NR_DIRECTORY];

  ibv_mr *cacheMR;
  void *cachePool;
  uint32_t cacheLKey;

  RemoteConnection *remoteInfo;

  ThreadConnection(uint16_t node_id, uint16_t threadID, void *cachePool, uint64_t cacheSize,
                   uint32_t machineNR, RemoteConnection *remoteInfo,
                   const std::string &nic_name, RCMapping *rc_mapping);

  void sendMessage(RpcMessage *m, uint16_t node_id, uint16_t thread_id);
};

#endif /* __THREADCONNECTION_H__ */
