#include "rowan/RowanServer.h"
#include "rdma/mlx5_defs.h"

RowanServer::RowanServer(DSM *dsm)
    : dsm(dsm) {
}

// for test rowan's raw performance
void RowanServer::raw_manage_srq() {
  auto ep = dsm->get_server_endpoint(0);
  auto srq = ep->get_srq();

  auto mlx5_srq = get_mlx5_srq(srq);
  uint16_t mlx5_srq_head = 0;

  auto buf_start = ep->get_recv_buf(kSrqBufferCnt * ep->kStrideBufSize);
  for (size_t k = 0; k < kSrqBufferCnt; ++k) {
    auto seg = buf_start + k * ep->kStrideBufSize;
    *seg = 0;
    compiler_barrier();
    ep->recv(seg, ep->kStrideBufSize);
  }
  printf("init mp srq\n");

  uint64_t head = 0;
  while (true) {
    uint64_t next_header = (head + 1) % kSrqBufferCnt;
    volatile char *poll = buf_start + next_header * ep->kStrideBufSize;
    if (*poll != 0) { // this segment is being used
      mlx5_free_srq_wqe(mlx5_srq, mlx5_srq_head);
      mlx5_srq_head = (mlx5_srq_head + 1) % (2 * ep->kMpSrqSize);

      auto seg = buf_start + head * ep->kStrideBufSize;
      *seg = 0;
      compiler_barrier();

      ep->recv(seg, ep->kStrideBufSize);
      head = next_header;
    }
  }
}