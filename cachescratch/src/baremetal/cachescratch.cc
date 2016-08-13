//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <ebbrt/Acpi.h>
#include <ebbrt/Cpu.h>
#include <ebbrt/Debug.h>
#include <ebbrt/EventManager.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/Multiboot.h>
#include <ebbrt/SpinBarrier.h>
#include <ebbrt/UniqueIOBuf.h>

#include "../common/clock.h"
#include "../common/worker.h"

int nthreads = 1;       // Default number of threads.
int niterations = 1000; // Default number of iterations.
int repetitions = 1000000;
int objSize = 1;

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

void AppMain() {
  int ret = 0;
  int i;

  auto cmdline = std::string(ebbrt::multiboot::CmdLine());
  ret = ParseInt(cmdline, "niterations=");
  if (ret != -1)
    niterations = ret;

  ret = ParseInt(cmdline, "repetitions=");
  if (ret != -1)
    repetitions = ret;

  ret = ParseInt(cmdline, "nthreads=");
  if (ret != -1)
    nthreads = ret;

  ret = ParseInt(cmdline, "objSize=");
  if (ret != -1)
    objSize = ret;

  uint64_t ns;
  MyTimer tmr;

  ebbrt::kprintf("Running cache-scratch for %d threads, %d iterations,"
                 " %d repetitions and %d objSize...\n",
                 nthreads, niterations, repetitions, objSize);

  size_t ncpus = nthreads; // ebbrt::Cpu::Count();
  static ebbrt::SpinBarrier bar(ncpus);
  ebbrt::EventManager::EventContext context;
  std::atomic<size_t> count(0);
  size_t theCpu = ebbrt::Cpu::GetMine();

  // Allocate nthreads objects and distribute them among the threads.
  char **objs = new char *[nthreads];
  for (i = 0; i < nthreads; i++) {
    objs[i] = new char[objSize];
  }
  workerArg *w = new workerArg[nthreads];

  ns_start(tmr);

  for (i = 0; i < (int)ncpus; i++) {
    // spawn jobs on each core using SpawnRemote
    ebbrt::event_manager->SpawnRemote(
        [theCpu, ncpus, &count, &context, i, &objs, &w]() {
          w[i] =
              workerArg(objs[i], objSize, repetitions / nthreads, niterations);

          worker((void *)&w[i]);

          count++;
          bar.Wait();
          while (count < (size_t)ncpus)
            ;
          if (ebbrt::Cpu::GetMine() == theCpu) {
            ebbrt::event_manager->ActivateContext(std::move(context));
          }
        },
        indexToCPU(i));
  }
  ebbrt::event_manager->SaveContext(context);

  ns = ns_stop(tmr);

  ebbrt::kprintf("hoardthreadtest time elapsed = %" PRIu64 " ~ns\n", ns);

  ebbrt::acpi::PowerOff();
}
