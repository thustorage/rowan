#include "dsm/DSM.h"
#include "rowan/RowanServer.h"

// ./rowan_server client_nr
int main(int argc, char **argv) {

  if (argc != 2) {
    fprintf(stderr, "Usage: ./server client_nr\n");
    exit(-1);
  }

  uint32_t client_nr = std::atoi(argv[1]);

  ClusterInfo cluster;
  cluster.server_cnt = 1;
  cluster.client_cnt = client_nr;

  DSMConfig config(cluster, 128 * define::MB, 128 * define::MB,
                   NodeType::SERVER);
  config.nic_name = "mlx5_0";
  auto dsm = DSM::get_instance(config);

  if (dsm->get_my_node_id() != 0) {
    printf("error: the server id should be 0\n");
    exit(-1);
  }

  auto server = new RowanServer(dsm);

  server->raw_manage_srq();

  while (true) {
  }
}