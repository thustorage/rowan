#include "rdma/mlx5_defs.h"

#include <arpa/inet.h>

static inline struct mlx5_srq *to_msrq(struct ibv_srq *ibsrq) {
  struct verbs_srq *vsrq = (struct verbs_srq *)ibsrq;

  return container_of(vsrq, struct mlx5_srq, vsrq);
}

struct mlx5_srq *get_mlx5_srq(struct ibv_srq *ibsrq) {
  if (ibsrq->handle == LEGACY_XRC_SRQ_HANDLE)
    ibsrq = (struct ibv_srq *)(((struct ibv_srq_legacy *)ibsrq)->ibv_srq);

  mlx5_srq *srq = to_msrq(ibsrq);

  return srq;
}

static void *get_wqe(struct mlx5_srq *srq, int n) {
  return (char *)srq->buf.buf + (n << srq->wqe_shift);
}

void mlx5_free_srq_wqe(struct mlx5_srq *srq, int ind) {
  struct mlx5_wqe_srq_next_seg *next;

  next = (struct mlx5_wqe_srq_next_seg *)get_wqe(srq, srq->tail);
  next->next_wqe_index = htons(ind);
  srq->tail = ind;
}
