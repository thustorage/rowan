#include "rowan/RowanClient.h"

RowanClient::RowanClient(DSM *dsm) : dsm(dsm) {
  if (dsm->get_my_thread_id() != -1) {
    printf("error: the constructor should be called in a new thread %d\n",
           dsm->get_my_thread_id());
  }
  dsm->register_thread(kBufferCnt * MESSAGE_SIZE);

  send_buf = dsm->get_rdma_buffer();
  cur_send = 0;
  send_counter = 0;
}

RpcMessage *RowanClient::get_send_buf() {
  auto res = (RpcMessage *)(send_buf + cur_send * MESSAGE_SIZE);
  cur_send = (cur_send + 1) % kBufferCnt;
  res->clear();
  return res;
}