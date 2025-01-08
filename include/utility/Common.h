#ifndef __COMMON_H__
#define __COMMON_H__

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <atomic>
#include <bitset>

#include "utility/Debug.h"
#include "utility/HugePageAlloc.h"
#include "utility/Slice.h"
#include "utility/WRLock.h"
#include "utility/Asm.h"

namespace define {

constexpr uint64_t KB = 1024ull;
constexpr uint64_t MB = 1024ull * 1024;
constexpr uint64_t GB = 1024ull * MB;

constexpr uint64_t ns2ms = 1000ull * 1000ull;
constexpr uint64_t ns2s = ns2ms * 1000ull;

} // namespace define




#endif /* __COMMON_H__ */
