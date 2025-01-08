#pragma once

#include "RowanCommon.h"
#include "dsm/DSM.h"

const static uint16_t kBufferCnt = 16;
class RowanClient {
private:
  DSM *dsm;
  char *send_buf;
  uint64_t cur_send;
  uint64_t send_counter;

  const static int kBufferCnt = 8;

public:
  RowanClient(DSM *dsm);
  ~RowanClient() {}
  RpcMessage *get_send_buf();
};