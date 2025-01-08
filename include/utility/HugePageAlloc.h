#ifndef __HUGEPAGEALLOC_H__
#define __HUGEPAGEALLOC_H__

#include <cstdint>
#include <cstdio>
#include <fcntl.h>
#include <sys/stat.h>

#include <memory.h>
#include <sys/mman.h>

#include "Debug.h"
#include <string>

char *getIP();

inline void *page_alloc(size_t size) {

  void *res = mmap(NULL, size, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
  if (res == MAP_FAILED) {
    Debug::notifyError("[1] %s mmap failed!\n", getIP());
  }

  return res;
}

inline void page_free(void *ptr, size_t size) { munmap(ptr, size); }


#endif /* __HUGEPAGEALLOC_H__ */
