
#pragma once

#include "Config.h"
#include "utility/Slice.h"
#include <cstring>

using RpcType = uint8_t;

struct Cursor {
  uint32_t off;

  Cursor() : off(0) {}
};

struct RpcMessage {
  uint8_t magic;
  RpcType type;
  NodeIDType node_id;
  ThreadIDType t_id;
  uint16_t msg_size{0};
  char msg[0];

  void print() {
    printf("type %d, [%d %d] [size: %d] ", type, node_id, t_id, msg_size);
    for (size_t i = 0; i < msg_size; ++i) {
      printf("%c-", msg[i]);
    }
    printf("\n");
  }

  size_t size() const { return sizeof(RpcMessage) + this->msg_size; }

  void clear() { msg_size = 0; }

  void set_type(RpcType t) { type = t; }

  template <class T> void add(const T &v) {
    *(T *)(msg + msg_size) = v;
    msg_size += sizeof(T);
  }

  void add_string_wo_size(const char *s, uint32_t k) {
    memcpy(msg + msg_size, s, k);
    msg_size += k;
  }

  void add_string_wo_size(Slice s) { add_string_wo_size(s.s, s.len); }

  void add_string(const char *s, uint32_t k) {
    add<uint32_t>(k);
    memcpy(msg + msg_size, s, k);
    msg_size += k;
  }

  void add_string(Slice s) { add_string(s.s, s.len); }

  template <class T> T get(Cursor &cur) {
    T k = *(T *)(msg + cur.off);
    cur.off += sizeof(T);

    return k;
  }

  Slice get_string_wo_size(Cursor &cur, size_t size) {
    Slice res(msg + cur.off, size);
    cur.off += size;

    return res;
  }

  Slice get_string(Cursor &cur) {
    uint32_t k = *(uint32_t *)(msg + cur.off);
    Slice res(msg + cur.off + sizeof(uint32_t), k);

    cur.off += sizeof(uint32_t) + k;

    return res;
  }

  RpcMessage *th(size_t k) { // msg array
    return (RpcMessage *)((char *)this + MESSAGE_SIZE * k);
  }

  static RpcMessage *get_new_msg() {
    RpcMessage *m = (RpcMessage *)malloc(MESSAGE_SIZE);
    m->msg_size = 0;

    return m;
  }

  static RpcMessage *get_new_msg_arr(size_t k) {
    RpcMessage *m = (RpcMessage *)malloc(MESSAGE_SIZE * k);

    for (size_t i = 0; i < k; ++i) {
      auto *e = (RpcMessage *)((char *)m + MESSAGE_SIZE * i);
      e->msg_size = 0;
    }

    return m;
  }

  static void put_new_msg(RpcMessage *m) { free(m); }
  static void put_new_msg_arr(RpcMessage *m) { free(m); }

private:
  RpcMessage() { printf("why call this constructor?\n"); }

} __attribute__((packed));