#if !defined(_ASM_H_)
#define _ASM_H_

#include <cstdint>

namespace define {
constexpr uint16_t kCacheLineSize = 64;
}

inline void clflush(void *addr) {
#ifdef __x86_64__
  asm volatile("clflush %0" : "+m"(*(volatile char *)(addr)));
#endif
}

inline void clwb(void *addr) {
#ifdef __x86_64__
  asm volatile(".byte 0x66; xsaveopt %0" : "+m"(*(volatile char *)(addr)));
#endif
}

inline void clflushopt(void *addr) {
#ifdef __x86_64__
  asm volatile(".byte 0x66; clflush %0" : "+m"(*(volatile char *)(addr)));
#endif
}

inline void persistent_barrier() { 
  #ifdef __x86_64__
  asm volatile("sfence\n" : :); 
  #endif
  }

inline void compiler_barrier() { asm volatile("" ::: "memory"); }

inline void clflush_range(void *des, size_t size) {
  char *addr = (char *)des;
  size = size + ((uint64_t)(addr) & (define::kCacheLineSize - 1));
  for (size_t i = 0; i < size; i += define::kCacheLineSize) {
    clflush(addr + i);
  }
}

inline void clwb_range(void *des, size_t size) {
  char *addr = (char *)des;
  size = size + ((uint64_t)(addr) & (define::kCacheLineSize - 1));
  for (size_t i = 0; i < size; i += define::kCacheLineSize) {
    clwb(addr + i);
  }
  persistent_barrier();
}

inline void clflushopt_range(void *des, size_t size) {
  char *addr = (char *)des;
  size = size + ((uint64_t)(addr) & (define::kCacheLineSize - 1));
  for (size_t i = 0; i < size; i += define::kCacheLineSize) {
    clflushopt(addr + i);
  }
  persistent_barrier();
}

inline unsigned long long asm_rdtsc(void) {
#ifdef __x86_64__
  unsigned hi, lo;
  __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
  return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
#else
  return 1;
#endif
}

inline uint64_t next_cache_line(uint64_t v) {
  return (v + define::kCacheLineSize - 1) & (~(define::kCacheLineSize - 1));
}

#endif