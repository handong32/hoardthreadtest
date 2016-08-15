#ifndef __MY_TIMER__
#define __MY_TIMER__
#include <inttypes.h>
#include <ebbrt/Clock.h>
typedef ebbrt::clock::HighResTimer MyTimer;
static inline void ns_start(MyTimer &t) { t.tick(); }
static inline uint64_t ns_stop(MyTimer &t) { return t.tock().count(); }
#endif
