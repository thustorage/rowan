#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <cstdint>
#include <string>

namespace config {
  constexpr int kMaxNetThread = 36;
}

using NodeIDType = uint8_t;
using ThreadIDType = uint8_t;

#define MESSAGE_SIZE 4096 // byte
#define RPC_QUEUE_SIZE 128

// number of RC context per server
#define NR_DIRECTORY 1

struct ClusterInfo {
  uint32_t server_cnt;
  uint32_t client_cnt;

  ClusterInfo(uint32_t n = 2) : server_cnt(n), client_cnt(0) {}
};

enum class NodeType { SERVER, CLIENT };

class DSMConfig {
public:

  ClusterInfo cluster_info;
  uint32_t machine_cnt;

  uint64_t dsm_addr; // in bytes
  uint64_t dsm_size;  // in bytes

  uint64_t buf_addr; // in bytes
  uint64_t buf_size; // in bytes

  NodeType node_type;

  std::string nic_name;

  DSMConfig(ClusterInfo cluster_info, uint64_t dsmSize = 4096, uint64_t bufferSize = 4096,
   NodeType node_type = NodeType::SERVER)
      : cluster_info(cluster_info), dsm_size(dsmSize), buf_size(bufferSize),
        node_type(node_type) {
    machine_cnt = cluster_info.client_cnt + cluster_info.server_cnt;

    dsm_addr = 0;
    buf_addr = 0;
  }
};

#endif /* __CONFIG_H__ */
