#include "dsm/DSM.h"
#include "utility/Timer.h"
#include "utility/Zipf.h"
#include "utility/System.h"

#include <thread>

const int kMaxTestThread = 24;
const int kBucketPerThread = 32;

std::thread th[kMaxTestThread];
uint64_t tp_counter[kMaxTestThread][8];
uint64_t tp_write_counter[kMaxTestThread][8];
DSM *dsm;

int node_nr, my_node;
int thread_nr;

void rpc_test(int node_id, int thread_id) {

  bind_core(thread_id);
  dsm->register_thread();

  auto *m = RpcMessage::get_new_msg();

  dsm->barrier("XXX");

  while (true) {
    if (dsm->get_my_node_id() == 0) {
      printf("call \n");
      dsm->ud_rpc_call(m, 1, 0);
      printf("call 2\n");
      auto mm = dsm->ud_rpc_wait();
      printf("XXX from node %d, t_id %d\n", mm->node_id, mm->t_id);
    } else {
      printf("recv \n");
      auto mm = dsm->ud_rpc_wait();
      printf("YYY from node %d, t_id %d\n", mm->node_id, mm->t_id);
      dsm->ud_rpc_call(m, mm->node_id, mm->t_id);
    }
  }
}

void read_args(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: ./rpc_test node_nr thread_nr\n");
    exit(-1);
  }

  node_nr = std::atoi(argv[1]);
  thread_nr = std::atoi(argv[2]);

  printf("node_nr [%d], thread_nr [%d]\n", node_nr, thread_nr);
}

extern uint64_t dir_counter;

int main(int argc, char **argv) {

  bind_core(0);

  read_args(argc, argv);
  
  ClusterInfo cluster(node_nr);
  DSMConfig config(cluster, 4 * define::KB, 4 * define::KB);
  config.nic_name = "mlx5_0";
  dsm = DSM::get_instance(config);

  for (int i = 0; i < thread_nr; ++i) {
    th[i] = std::thread(rpc_test, dsm->get_my_node_id(), i);
  }

  while (true) {
  }
}