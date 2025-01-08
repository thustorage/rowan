#if !defined(_SYSTEM_H_)
#define _SYSTEM_H_

#include <cstdint>

constexpr int kLogicCoreCnt = 36;
constexpr int kMaxSocketCnt = 2;

void bind_core(uint16_t core);
void auto_bind_core(int ServerConfig = 0);
char *getIP();
char *getMac();

#endif // _SYSTEM_H_
