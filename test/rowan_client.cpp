#include "dsm/DSM.h"
#include "rowan/RowanClient.h"
#include "utility/Timer.h"

DSM *dsm;
uint32_t client_thread_nr;

uint64_t tp_counter[64][8];

void test_rowan(int thread_id) {
  auto client = new RowanClient(dsm);
  size_t kBatchSize = 6;
  for (size_t k = 0; k < kBatchSize; ++k) {
    auto request = client->get_send_buf();
    request->magic = 1;
    dsm->send((char *)request, request->size(), 0, true);
  }

  while (true) {
    dsm->poll_rdma_cq(1);
    auto request = client->get_send_buf();
    request->magic = 1;
    dsm->send((char *)request, request->size(), 0, true);
    tp_counter[thread_id][0]++;
  }
}

// ./client client_nr client_thread_nr
int main(int argc, char **argv) {

  if (argc != 3) {
    fprintf(stderr, "Usage: ./client client_nr client_thread_nr\n");
    exit(-1);
  }

  uint32_t client_nr = std::atoi(argv[1]);
  client_thread_nr = std::atoi(argv[2]);

  ClusterInfo cluster;
  cluster.server_cnt = 1;
  cluster.client_cnt = client_nr;

  DSMConfig config(cluster, 128 * define::MB, 128 * define::MB,
                   NodeType::CLIENT);
  config.nic_name = "mlx5_0";
  dsm = DSM::get_instance(config);

  sleep(3);
  for (size_t i = 0; i < client_thread_nr; ++i) {
    new std::thread(test_rowan, i);
  }

  Timer timer;
  uint64_t pre_tp = 0;
  timer.begin();
  while (true) {

    sleep(1);

    auto ns = timer.end();

    uint64_t tp = 0;

    for (size_t i = 0; i < client_thread_nr; ++i) {
      tp += tp_counter[i][0];
    }

    double t_tp = (tp - pre_tp) * 1.0 / (1.0 * ns / 1000 / 1000 / 1000);
    pre_tp = tp;

    timer.begin();
    printf("IOPS: %lf  --- all %ld\n", t_tp, tp);
  }

  while (true) {
  }
}