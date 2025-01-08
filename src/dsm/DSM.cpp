
#include "dsm/DSM.h"
#include "utility/HugePageAlloc.h"

#include "dsm/DSMKeeper.h"

#include <algorithm>
#include <iostream>

thread_local int DSM::thread_id = -1;
thread_local ThreadConnection *DSM::iCon = nullptr;
thread_local char *DSM::rdma_buffer = nullptr;

DSM *DSM::get_instance(const DSMConfig &conf) {
  static DSM dsm(conf);
  return &dsm;
}

DSM::DSM(const DSMConfig &conf) : conf(conf), appID(0) {
  Debug::notifyInfo("Machine Count: %d [server: %d, client: %d]\n",
                    conf.machine_cnt, conf.cluster_info.server_cnt,
                    conf.cluster_info.client_cnt);
  
  rc_mapping = new RCMapping(conf.machine_cnt);
  remoteInfo = new RemoteConnection[conf.machine_cnt];
  keeper = new DSMKeeper(thCon, dirCon, remoteInfo, conf.machine_cnt);
  my_node_id = keeper->getMyNodeID();

  if (conf.node_type == NodeType::CLIENT &&
      my_node_id < conf.cluster_info.server_cnt) {
    printf("I am client but has error node id %d %d\n", my_node_id,
           conf.cluster_info.server_cnt);
    exit(-1);
  }

  this->conf.dsm_addr = (uint64_t)page_alloc(this->conf.dsm_size);
  this->conf.buf_addr = (uint64_t)page_alloc(this->conf.buf_size);

  Debug::notifyInfo("shared memory size: %ldB, 0x%lx", this->conf.dsm_size,
                    this->conf.dsm_addr);
  Debug::notifyInfo("buffer size: %dB", this->conf.buf_size);

  initRDMAConnection(conf.nic_name);

  keeper->barrier("DSM-init", conf.machine_cnt);
}

DSM::~DSM() {}

void DSM::register_thread(uint64_t per_thread_buf_size) {
  if (thread_id != -1) {
    return;
  }

  thread_id = appID.fetch_add(1);
  iCon = thCon[thread_id];
  iCon->message->initRecv();
  iCon->message->initSend();

  rdma_buffer =
      (char *)conf.buf_addr + thread_id * per_thread_buf_size;
}

void DSM::initRDMAConnection(const std::string &nic_name) {

  for (int i = 0; i < config::kMaxNetThread; ++i) {
    thCon[i] = new ThreadConnection(my_node_id, i, (void *)conf.buf_addr,
                                    conf.buf_size,
                                    conf.machine_cnt, remoteInfo, nic_name, rc_mapping);
  }

  for (int i = 0; i < NR_DIRECTORY; ++i) {
    dirCon[i] = new DirectoryConnection(
        i, (void *)this->conf.dsm_addr, conf.dsm_size,
        conf.machine_cnt, remoteInfo, (void *)conf.buf_addr,
        conf.buf_size, nic_name, rc_mapping);
  }
  std::cerr << std::flush;
  std::cout << std::flush;

  keeper->run();
}

///////// send
void DSM::send(char *buffer, size_t size, uint16_t node_id, bool signal) {
  rdmaSend(iCon->data[iCon->target_dirID][node_id], (uint64_t)buffer, size,
           iCon->cacheLKey, -1, signal);
}

void DSM::send_sync(char *buffer, size_t size, uint16_t node_id) {
  send(buffer, size, node_id, true);
  ibv_wc wc;
  pollWithCQ(iCon->cq, 1, &wc);
}

///////// read
void DSM::read(const char *buffer, uint16_t node_id, uint64_t addr, size_t size,
               uint64_t wr_id, bool signal) {
  rdmaRead(iCon->data[0][node_id], (uint64_t)buffer, addr, size,
           iCon->cacheLKey, remoteInfo[node_id].dsmRKey[0], signal, wr_id);
}

void DSM::read_sync(const char *buffer, uint16_t node_id, uint64_t addr,
                    size_t size, uint64_t wr_id) {
  read(buffer, node_id, addr, size, wr_id, true);

  ibv_wc wc;
  pollWithCQ(iCon->cq, 1, &wc);
}

///////// write
void DSM::write(const char *buffer, uint16_t node_id, uint64_t addr,
                size_t size, uint64_t wr_id, bool signal) {
  rdmaWrite(iCon->data[0][node_id], (uint64_t)buffer, addr, size,
            iCon->cacheLKey, remoteInfo[node_id].dsmRKey[0], -1, signal, wr_id);
}

void DSM::write_sync(const char *buffer, uint16_t node_id, uint64_t addr,
                     size_t size, uint64_t wr_id) {
  write(buffer, node_id, addr, size, wr_id, true);

  ibv_wc wc;
  pollWithCQ(iCon->cq, 1, &wc);
}

///////// cas
void DSM::cas(uint16_t node_id, uint64_t addr, uint64_t equal, uint64_t val,
              uint64_t *rdma_buffer, uint64_t wr_id, bool signal) {
  rdmaCompareAndSwap(iCon->data[0][node_id], (uint64_t)rdma_buffer, addr, equal,
                     val, iCon->cacheLKey, remoteInfo[node_id].dsmRKey[0],
                     signal);
}

bool DSM::cas_sync(uint16_t node_id, uint64_t addr, uint64_t equal,
                   uint64_t val, uint64_t *rdma_buffer, uint64_t wr_id) {
  cas(node_id, addr, equal, val, rdma_buffer, wr_id, true);
  ibv_wc wc;
  pollWithCQ(iCon->cq, 1, &wc);

  return equal == *rdma_buffer;
}

///////// faa
void DSM::faa(uint16_t node_id, uint64_t addr, uint64_t add,
              uint64_t *rdma_buffer, uint64_t wr_id, bool signal) {
  rdmaFetchAndAdd(iCon->data[0][node_id], (uint64_t)rdma_buffer, addr, add,
                  iCon->cacheLKey, remoteInfo[node_id].dsmRKey[0], signal);
}

void DSM::faa_sync(uint16_t node_id, uint64_t addr, uint64_t add,
                   uint64_t *rdma_buffer, uint64_t wr_id) {
  faa(node_id, addr, add, rdma_buffer, wr_id, true);

  ibv_wc wc;
  pollWithCQ(iCon->cq, 1, &wc);
}

///////// poll
void DSM::poll_rdma_cq(int count) {
  ibv_wc wc;
  pollWithCQ(iCon->cq, count, &wc);
}