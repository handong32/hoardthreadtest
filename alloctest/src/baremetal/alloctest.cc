//          Copyright Boston University SESA Group 2013 - 2014.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
#include <ebbrt/Clock.h>
#include <ebbrt/Cpu.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/ExplicitlyConstructed.h>
#include <ebbrt/GeneralPurposeAllocator.h>
#include <ebbrt/Rdtsc.h>
#include <ebbrt/SpinBarrier.h>

#define MULTICORE

template <typename Iter>
void print_stats(
    Iter begin, Iter end,
    typename std::enable_if<
        std::is_same<typename std::iterator_traits<Iter>::iterator_category,
                     std::random_access_iterator_tag>::value>::type * = 0) {
  std::sort(begin, end);
  auto min = *std::min_element(begin, end);
  auto max = *std::max_element(begin, end);
  auto size = end - begin;
  auto firstq = *(begin + (size * .25));
  auto median = *(begin + (size * .5));
  auto thirdq = *(begin + (size * .75));
  auto sum = std::accumulate(begin, end, 0.0);
  auto mean = sum / size;

  std::vector<double> diff(size);
  std::transform(begin, end, diff.begin(),
                 [mean](uint64_t s) { return s - mean; });
  auto sq_sum = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
  auto stddev = std::sqrt(sq_sum / size);

  ebbrt::kprintf("mean: %f\n", mean);
  ebbrt::kprintf("stddev: %f\n", stddev);
  ebbrt::kprintf("min: %llu\n", min);
  ebbrt::kprintf("25%%: %llu\n", firstq);
  ebbrt::kprintf("50%%: %llu\n", median);
  ebbrt::kprintf("75%%: %llu\n", thirdq);
  ebbrt::kprintf("max: %llu\n", max);
}

namespace {
ebbrt::ExplicitlyConstructed<ebbrt::SpinBarrier> barrier;
ebbrt::SpinLock lock;

//uint64_t rdtsc_clock() { return ebbrt::rdtsc(); }

//uint64_t rdtscp_clock() { return ebbrt::rdtscp(); }

/*uint64_t wall_clock() {
  std::chrono::nanoseconds ns = ebbrt::clock::Wall::Now().time_since_epoch();
  return ns.count();
  }*/

//void *__attribute__((noinline)) my_malloc(size_t sz) { return malloc(sz); }

//void __attribute__((noinline)) my_free(void *ptr) { free(ptr); }
}

#define CLOCK_RESOLUTION

size_t niterations = 50; // Default number of iterations.
size_t nobjects = 30000; // Default number of objects.
size_t nthreads = 1;     // Default number of threads.
size_t work = 0;         // Default number of loop iterations.
size_t objSize = 1;

class Foo {
public:
  Foo(void) : x(14), y(29) {}
  int x;
  int y;
};

size_t ParseArg(std::string str, std::string mstr) {
  auto test = str.find(mstr);
  if (test == std::string::npos)
    return 0;
  auto test2 = str.substr(test);

  auto test3 = test2.find(";");
  if (test3 == std::string::npos)
    return 0;
  auto test4 = test2.substr(0, test3);

  auto test5 = test4.find("=");
  if (test5 == std::string::npos)
    return 0;
  auto test6 = test4.substr(test5 + 1);
  // ebbrt::kprintf("%d\n", atoi(test6.c_str()));
  return atoi(test6.c_str());
}

void Work() {
  size_t i, j;
  Foo **a;
  const constexpr size_t kSamples = 1000000;
  std::vector<uint64_t> samples(kSamples);

  barrier->Wait();

  for (auto &sample : samples) {
    ebbrt::clock::HighResTimer timer;
    timer.tick();

    a = new Foo *[nobjects / nthreads];

    for (j = 0; j < niterations; j++) {

      for (i = 0; i < (nobjects / nthreads); i++) {
        a[i] = new Foo[objSize];
#if 1
        for (volatile size_t d = 0; d < work; d++) {
          volatile size_t f = 1;
          f = f + f;
          f = f * f;
          f = f + f;
          f = f * f;
        }
#endif
        assert(a[i]);
      }

      for (i = 0; i < (nobjects / nthreads); i++) {
        delete[] a[i];
#if 1
        for (volatile size_t d = 0; d < work; d++) {
          volatile size_t f = 1;
          f = f + f;
          f = f * f;
          f = f + f;
          f = f * f;
        }
#endif
      }
    }
    delete[] a;
    /*for (size_t i = 0; i < 10; ++i) {
      auto f = malloc(8);
      *(volatile uint64_t*)f;
      free(f);
      }*/

    sample = timer.tock().count();
  }

  barrier->Wait();

  {
    std::lock_guard<ebbrt::SpinLock> guard(lock);
    ebbrt::kprintf("<%u>: new/delete latency:\n",
                   static_cast<size_t>(ebbrt::Cpu::GetMine()));
    print_stats(samples.begin(), samples.end());
  }

  barrier->Wait();

  /*for (auto &sample : samples) {
    ebbrt::clock::HighResTimer timer;
    timer.tick();
    for (size_t i = 0; i < 10; ++i) {
      ebbrt::event_manager->Spawn([]() { asm volatile(""); }, true);
    }
    sample = timer.tock().count();
    ebbrt::event_manager->Clear();
  }

  barrier->Wait();

  {
    std::lock_guard<ebbrt::SpinLock> guard(lock);
    ebbrt::kprintf("<%u>: 10 Spawn latency:\n",
                   static_cast<size_t>(ebbrt::Cpu::GetMine()));
    print_stats(samples.begin(), samples.end());
  }

  barrier->Wait();*/
}

void AppMain() {
#ifdef MULTICORE
  barrier.construct(ebbrt::Cpu::Count());
#else
  barrier.construct(1);
#endif

  size_t ret = 0;
  auto cmdline = std::string(ebbrt::multiboot::CmdLine());
  ret = ParseArg(cmdline, "niterations=");
  if (ret != 0)
    niterations = ret;

  ret = ParseArg(cmdline, "nobjects=");
  if (ret != 0)
    nobjects = ret;

  ret = ParseArg(cmdline, "nthreads=");
  if (ret != 0)
    nthreads = ret;

  ret = ParseArg(cmdline, "work=");
  if (ret != 0)
    work = ret;

  ret = ParseArg(cmdline, "objSize=");
  if (ret != 0)
    objSize = ret;

  ebbrt::kprintf("Running alloc threadtest for %lu threads, %lu iterations,"
                 " %lu objects, %lu work and %lu objSize...\n",
                 nthreads, niterations, nobjects, work, objSize);

#ifdef MULTICORE
  for (size_t i = 1; i < ebbrt::Cpu::Count(); ++i) {
    ebbrt::event_manager->SpawnRemote(Work, i);
  }
#endif
  Work();
}
