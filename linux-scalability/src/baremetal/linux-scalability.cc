/*
 *  malloc-test
 *  cel - Thu Jan  7 15:49:16 EST 1999
 *
 *  Benchmark libc's malloc, and check how well it
 *  can handle malloc requests from multiple threads.
 *
 *  Syntax:
 *  malloc-test [ size [ iterations [ thread count ]]]
 *
 */

/*
 * June 9, 2013: Modified by Emery Berger to use barriers so all
 * threads start at the same time; added statistics gathering.
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <ebbrt/Acpi.h>
#include <ebbrt/Cpu.h>
#include <ebbrt/Debug.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/Multiboot.h>
#include <ebbrt/SpinBarrier.h>
#include <ebbrt/UniqueIOBuf.h>

#define USECSPERSEC 1000000
#define pthread_attr_default NULL
#define MAX_THREADS 50

double *executionTime;
void *run_test(void *);
void *dummy(unsigned);

static unsigned long size = 512;
static uint64_t iteration_count = 1000000;
static unsigned int thread_count = 1;

static size_t indexToCPU(size_t i) { return i; }

int ParseInt(std::string str, std::string mstr) {
  auto test = str.find(mstr);
  if (test == std::string::npos)
    return -1;
  auto test2 = str.substr(test);

  auto test3 = test2.find(";");
  if (test3 == std::string::npos)
    return -1;
  auto test4 = test2.substr(0, test3);

  auto test5 = test4.find("=");
  if (test5 == std::string::npos)
    return -1;
  auto test6 = test4.substr(test5 + 1);
  // ebbrt::kprintf("%d\n", atoi(test6.c_str()));
  return atoi(test6.c_str());
}

namespace {
ebbrt::ExplicitlyConstructed<ebbrt::SpinBarrier> bar;
}

#include "../common/worker.h"

void AppMain() {
  unsigned int i;
  int ret = 0;
  
  auto cmdline = std::string(ebbrt::multiboot::CmdLine());
  ret = ParseInt(cmdline, "niterations=");
  if (ret != -1)
    iteration_count = ret;

  ret = ParseInt(cmdline, "objSize=");
  if (ret != -1)
    size = ret;

  ret = ParseInt(cmdline, "nthreads=");
  if (ret != -1)
    thread_count = ret;

  ebbrt::kprintf("Object size: %lu, Iterations: %lu, Threads: %d\n", size,
                 iteration_count, thread_count);

  executionTime = (double *)malloc(sizeof(double) * thread_count);
  bar.construct(ebbrt::Cpu::Count());

  // pthread_barrier_init(&barrier, NULL, thread_count);
  size_t ncpus = ebbrt::Cpu::Count();
  ebbrt::EventManager::EventContext context;
  std::atomic<size_t> count(0);
  size_t theCpu = ebbrt::Cpu::GetMine();


  /*          * Invoke the tests          */
  ebbrt::kprintf("Starting test...\n");
  for (i = 0; i < thread_count; i++) {
    int *tid = (int *)malloc(sizeof(int));
    *tid = i;
    // spawn jobs on each core using SpawnRemote
    ebbrt::event_manager->SpawnRemote(
        [theCpu, ncpus, &count, &context, &i]() {

          run_test((void *)&i);

          count++;
          bar->Wait();
          while (count < (size_t)ncpus)
            ;
          if (ebbrt::Cpu::GetMine() == theCpu) {
            ebbrt::event_manager->ActivateContext(std::move(context));
          }
        },
        indexToCPU(i));
  }

  /*          * Wait for tests to finish          */
  ebbrt::event_manager->SaveContext(context);
 
  /* EDB: moved to outer loop. */
  /* Statistics gathering and reporting. */
  double sum = 0.0;
  double stddev = 0.0;
  double average;
  for (i = 0; i < thread_count; i++) {
    sum += executionTime[i];
  }
  average = sum / thread_count;
  for (i = 0; i < thread_count; i++) {
    double diff = executionTime[i] - average;
    stddev += diff * diff;
  }
  stddev = sqrt(stddev / (thread_count - 1));
  if (thread_count > 1) {
    ebbrt::kprintf(
        "Average execution time = %f seconds, standard deviation = %f.\n",
        average, stddev);
  } else {
    ebbrt::kprintf("Average execution time = %f seconds.\n", average);
  }
  exit(0);
}
