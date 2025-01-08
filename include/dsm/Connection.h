#ifndef __CONNECTION_H__
#define __CONNECTION_H__

#include "Config.h"
#include "RawMessageConnection.h"
#include "DirectoryConnection.h"
#include "ThreadConnection.h"

struct RemoteConnection {
  
  // directory
  uint64_t dsmBase;
  uint32_t dsmRKey[NR_DIRECTORY];

  // app thread
  uint32_t appRKey[config::kMaxNetThread];
  uint32_t appMessageQPN[config::kMaxNetThread];
  ibv_ah *appAh[config::kMaxNetThread];
};

#endif /* __CONNECTION_H__ */
