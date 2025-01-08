#if !defined(_RCMapping_H_)
#define _RCMapping_H_

#include "dsm/Config.h"
#include "utility/MurmurHash2.h"

#include <vector>

class RCMapping {
private:
  uint16_t machine_cnt;
  std::vector<std::vector<uint16_t>> thread_to_dir;
  std::vector<std::pair<uint16_t, uint16_t>> dir_to_thread[NR_DIRECTORY];

  uint16_t calacuate_target_dir(uint16_t node_id, uint16_t thread_id) {
    uint64_t unique_id = (node_id * config::kMaxNetThread) + thread_id;
    return unique_id % NR_DIRECTORY;
  }

public:
  RCMapping(uint16_t machine_cnt) : machine_cnt(machine_cnt) {
    thread_to_dir.resize(machine_cnt);
    for (size_t k = 0; k < machine_cnt; ++k) {
      thread_to_dir[k].resize(config::kMaxNetThread);
    }

    for (size_t i = 0; i < machine_cnt; ++i) {
      for (size_t k = 0; k < config::kMaxNetThread; ++k) {
        thread_to_dir[i][k] = calacuate_target_dir(i, k);
        dir_to_thread[thread_to_dir[i][k]].push_back({i, k});
      }
    }

    printf("rc mapping:\n");
    for (size_t k = 0; k < NR_DIRECTORY; ++k) {
      printf("dir %lu serves %lu threads\n", k, dir_to_thread[k].size());
    }
  }
  

  // from client threads
  uint16_t get_target_dir_id(uint16_t node_id, uint16_t thread_id) {
    return thread_to_dir[node_id][thread_id];
  }
  
  // to client threads
  std::vector<std::pair<uint16_t, uint16_t>>  *get_client_thread_list(uint16_t dir_id) {
    return &dir_to_thread[dir_id];
  }
};

#endif // _RCMapping_H_
