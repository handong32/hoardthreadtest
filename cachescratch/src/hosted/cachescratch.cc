//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

///-*-C++-*-//////////////////////////////////////////////////////////////////
//
// Hoard: A Fast, Scalable, and Memory-Efficient Allocator
//        for Shared-Memory Multiprocessors
// Contact author: Emery Berger, http://www.cs.umass.edu/~emery
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as
// published by the Free Software Foundation, http://www.fsf.org.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
//////////////////////////////////////////////////////////////////////////////

/**
 * @file cache-scratch.cpp
 *
 * cache-scratch is a benchmark that exercises a heap's cache locality.
 * An allocator that allows multiple threads to re-use the same small
 * object (possibly all in one cache-line) will scale poorly, while
 * an allocator like Hoard will exhibit near-linear scaling.
 *
 * Try the following (on a P-processor machine):
 *
 *  cache-scratch 1 1000 1 1000000
 *  cache-scratch P 1000 1 1000000
 *
 *  cache-scratch-hoard 1 1000 1 1000000
 *  cache-scratch-hoard P 1000 1 1000000
 *
 *  The ideal is a P-fold speedup.
*/

#include <stdio.h>
#include <stdlib.h>

#include "cpuinfo.h"
#include "fred.h"
//#include "timer.h"
#include "../common/clock.h"
#include "../common/worker.h"

int main(int argc, char *argv[]) {
  int nthreads = 1;      // Default number of threads.
  int iterations = 1000000; // Default number of iterations.
  int repetitions = 100;
  int objSize = 8;

  uint64_t ns;
  MyTimer tmr;

  if (argc > 4) {
    nthreads = atoi(argv[1]);
    iterations = atoi(argv[2]);
    objSize = atoi(argv[3]);
    repetitions = atoi(argv[4]);
  }

  printf("Running cache-scratch for %d threads, %d iterations,"
                 " %d repetitions and %d objSize...\n",
                 nthreads, iterations, repetitions, objSize);

  HL::Fred *threads = new HL::Fred[nthreads];
  HL::Fred::setConcurrency(HL::CPUInfo::getNumProcessors());

  workerArg *w = new workerArg[nthreads];

  int i;

  // Allocate nthreads objects and distribute them among the threads.
  char **objs = new char *[nthreads];
  for (i = 0; i < nthreads; i++) {
    objs[i] = new char[objSize];
  }

  // HL::Timer t;
  // t.start();
  ns_start(tmr);

  for (i = 0; i < nthreads; i++) {
    w[i] = workerArg(objs[i], objSize, repetitions / nthreads, iterations);
    threads[i].create(&worker, (void *)&w[i]);
  }
  for (i = 0; i < nthreads; i++) {
    threads[i].join();
  }
  // t.stop();
  ns = ns_stop(tmr);

  delete[] threads;
  delete[] objs;
  delete[] w;

  printf("cache-scratch time elapsed = %" PRIu64 " ~ns\n", ns);

  // printf ("Time elapsed = %f seconds.\n", (double) t);

  return 0;
}
