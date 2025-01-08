#ifndef __DSM_H__
#define __DSM_H__

#include <atomic>
#include <iostream>

#include "Config.h"
#include "Connection.h"
#include "DSMKeeper.h"
#include "RCMapping.h"
#include "dsm/RpcMessage.h"
#include "utility/Timer.h"

class DSMKeeper;
class Directory;

class DSM {
public:
  void register_thread(uint64_t per_thread_buf_size = 4096);
  static DSM *get_instance(const DSMConfig &conf);

  uint16_t get_my_node_id() { return my_node_id; }
  int get_my_thread_id() { return thread_id; }
  uint64_t get_remote_base_addr(uint16_t node_id) {
    return remoteInfo[node_id].dsmBase;
  }

  ThreadConnection *get_thread_conn() { return iCon; }

  // RDMA operations
  // buffer is registered memory (RDMABuffer cache)
  void send(char *buffer, size_t size, uint16_t node_id, bool signal = true);
  void send_sync(char *buffer, size_t size, uint16_t node_id);

  void read(const char *buffer, uint16_t node_id, uint64_t addr, size_t size,
            uint64_t wr_id = 0, bool signal = true);
  void read_sync(const char *buffer, uint16_t node_id, uint64_t addr,
                 size_t size, uint64_t wr_id = 0);

  void write(const char *buffer, uint16_t node_id, uint64_t addr, size_t size,
             uint64_t wr_id = 0, bool signal = true);
  void write_sync(const char *buffer, uint16_t node_id, uint64_t addr,
                  size_t size, uint64_t wr_id = 0);

  void cas(uint16_t node_id, uint64_t addr, uint64_t equal, uint64_t val,
           uint64_t *rdma_buffer, uint64_t wr_id = 0, bool signal = true);
  bool cas_sync(uint16_t node_id, uint64_t addr, uint64_t equal, uint64_t val,
                uint64_t *rdma_buffer, uint64_t wr_id = 0);

  void faa(uint16_t node_id, uint64_t addr, uint64_t add, uint64_t *rdma_buffer,
           uint64_t wr_id = 0, bool signal = true);
  void faa_sync(uint16_t node_id, uint64_t addr, uint64_t add,
                uint64_t *rdma_buffer, uint64_t wr_id = 0);

  void poll_rdma_cq(int count = 1);

  DSMConfig *get_conf() { return &conf; }

private:
  DSM(const DSMConfig &conf);
  ~DSM();

  void initRDMAConnection(const std::string &nic_name);

  DSMConfig conf;
  std::atomic_int appID;

  static thread_local int thread_id;
  static thread_local ThreadConnection *iCon;
  static thread_local char *rdma_buffer;

  uint32_t my_node_id;

  RCMapping *rc_mapping;
  RemoteConnection *remoteInfo;
  ThreadConnection *thCon[config::kMaxNetThread];
  DirectoryConnection *dirCon[NR_DIRECTORY];
  DSMKeeper *keeper;

  Directory *dirAgent[NR_DIRECTORY];

public:
  void barrier(const std::string &ss) { keeper->barrier(ss, conf.machine_cnt); }

  void barrier(const std::string &ss, int k) { keeper->barrier(ss, k); }

  char *get_dsm_base_addr() { return (char *)this->conf.dsm_addr; }

  char *get_rdma_buffer() { return rdma_buffer; }

  DirectoryConnection *get_server_endpoint(uint16_t rc_cxt_id) {
    return this->dirCon[rc_cxt_id];
  }

  void ud_rpc_call(RpcMessage *m, uint16_t node_id, uint16_t t_id,
                   Slice extra_data = Slice::Void()) {
    auto buffer = (RpcMessage *)iCon->message->getSendPool();

    assert(m->size() + 40 < MESSAGE_SIZE);
    memcpy(buffer, m, m->size());
    buffer->node_id = my_node_id;
    buffer->t_id = thread_id;

    if (extra_data.s) { // reduce a memcpy
      buffer->add_string(extra_data);
      iCon->sendMessage(buffer, node_id, t_id);
    } else {
      iCon->sendMessage(buffer, node_id, t_id);
    }
  }

  RpcMessage *ud_rpc_wait() {
    ibv_wc wc;

    pollWithCQ(iCon->ud_cq, 1, &wc);

    if (wc.opcode == IBV_WC_RECV) { // recv a message
      return (RpcMessage *)iCon->message->getMessage();
    }
    return nullptr;
  }
};

#endif /* __DSM_H__ */
