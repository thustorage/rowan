#pragma once

#include "RowanCommon.h"
#include "dsm/DSM.h"
#include "dsm/RpcMessage.h"

class RowanServer {

private:
  const static size_t kSrqBufferCnt = 36;
  static_assert(kSrqBufferCnt > 32, "XXX");

  DSM *dsm;
  
public:
  RowanServer(DSM *dsm);
  ~RowanServer() {}

  // for test mp srq perfermance
  void raw_manage_srq();
};