#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <thread>
#include <vector>
#include <assert.h>

class HighResTimer {
public:
  void tick() __attribute__((always_inline)) {
    asm volatile("cpuid;"
                 "rdtsc;"
                 "mov %%edx, %0;"
                 "mov %%eax, %1;"
                 : "=r"(cycles_high_), "=r"(cycles_low_)::"%rax", "%rbx",
                   "%rcx", "%rdx");
  }
  std::chrono::nanoseconds tock() __attribute__((always_inline)) {
    auto tsc = tock_tsc();
    return std::chrono::nanoseconds(
        static_cast<uint64_t>(tsc / once.ticks_per_nano));
  }

private:
  struct DoOnce {
    DoOnce() {
      HighResTimer timer;
      auto s = std::chrono::steady_clock::now();
      timer.tick();
      for (size_t i = 0; i < 1000000; i++) {
        asm volatile("");
      }
      auto elapsed_tsc = timer.tock_tsc();
      std::chrono::nanoseconds elapsed_steady =
          std::chrono::steady_clock::now() - s;
      ticks_per_nano = static_cast<double>(elapsed_tsc) /
                       static_cast<double>(elapsed_steady.count());
    }
    double ticks_per_nano;
  };

  static DoOnce once;

  uint64_t tock_tsc() __attribute__((always_inline)) {
    uint32_t cycles_high1, cycles_low1;
    asm volatile("rdtscp;"
                 "mov %%edx, %0;"
                 "mov %%eax, %1;"
                 "cpuid;"
                 : "=r"(cycles_high1), "=r"(cycles_low1)::"%rax", "%rbx",
                   "%rcx", "%rdx");
    auto start = (((uint64_t)cycles_high_ << 32) | cycles_low_);
    auto end = (((uint64_t)cycles_high1 << 32 | cycles_low1));
    return end - start;
  }

  uint32_t cycles_low_;
  uint32_t cycles_high_;
};

class SpinBarrier {
public:
  explicit SpinBarrier(unsigned int n) : n_(n), waiters_(0), completed_(0) {}

  void Wait() {
    auto step = completed_.load();

    if (waiters_.fetch_add(1) == n_ - 1) {
      waiters_.store(0);
      completed_.fetch_add(1);
    } else {
      while (completed_.load() == step) {
      }
    }
  }

private:
  const unsigned int n_;
  std::atomic<unsigned int> waiters_;
  std::atomic<unsigned int> completed_;
};

HighResTimer::DoOnce HighResTimer::once;

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

  printf("mean: %f\n", mean);
  printf("stddev: %f\n", stddev);
  printf("min: %lu\n", min);
  printf("25%%: %lu\n", firstq);
  printf("50%%: %lu\n", median);
  printf("75%%: %lu\n", thirdq);
  printf("max: %lu\n", max);
}

SpinBarrier *barrier;

size_t nthreads = 1;
size_t niterations = 50; // Default number of iterations.
size_t nobjects = 30000; // Default number of objects.
size_t work = 0;         // Default number of loop iterations.
size_t objSize = 1;

class Foo {
public:
  Foo(void) : x(14), y(29) {}

  size_t x;
  size_t y;
};

void Work(size_t tid) {
  static std::mutex lock;
  size_t i, j;
  Foo **a;
  
  const constexpr size_t kSamples = 1000000;
  std::vector<uint64_t> samples(kSamples);  
  
  cpu_set_t cpu_set;
  CPU_ZERO(&cpu_set);
  if (nthreads <= 12) {
    CPU_SET(tid * 2, &cpu_set);
  } else {
    CPU_SET(tid, &cpu_set);
  }
  pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);

  barrier->Wait();

  for (auto &sample : samples) {
    HighResTimer timer;
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

    /*for (size_t i = 0; i < kIter; ++i) {
      auto f = malloc(8);
      *(volatile uint64_t *)f;
      free(f);
      }*/
    sample = timer.tock().count();
  }

  barrier->Wait();

  {
    std::lock_guard<std::mutex> guard(lock);
    printf("<%lu>: Alloc/Free Latency:\n", tid);
    print_stats(samples.begin(), samples.end());
  }

  barrier->Wait();
}

int main(int argc, char **argv) {
  if (argc >= 2) {
    nthreads = atoi(argv[1]);
  }

  if (argc >= 3) {
    niterations = atoi(argv[2]);
  }

  if (argc >= 4) {
    nobjects = atoi(argv[3]);
  }

  if (argc >= 5) {
    work = atoi(argv[4]);
  }

  if (argc >= 6) {
    objSize = atoi(argv[5]);
  }

  barrier = new SpinBarrier(nthreads);

  std::vector<std::thread> threads(nthreads);
  for (size_t i = 1; i < nthreads; ++i) {
    threads[i - 1] = std::thread([i]() { Work(i); });
  }
  Work(0);
  for (size_t i = 1; i < nthreads; ++i) {
    threads[i - 1].join();
  }

  return 0;
}
