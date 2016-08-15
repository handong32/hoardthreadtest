///-*-C++-*-//////////////////////////////////////////////////////////////////
//
// Hoard: A Fast, Scalable, and Memory-Efficient Allocator
//        for Shared-Memory Multiprocessors
// Contact author: Emery Berger, http://www.cs.umass.edu/~emery
//
// Copyright (c) 1998-2003, The University of Texas at Austin.
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
 * @file  cache-thrash.cpp
 * @brief cache-thrash is a benchmark that exercises a heap's cache-locality.
 *
 * Try the following (on a P-processor machine):
 *
 *  cache-thrash 1 1000 1 1000000
 *  cache-thrash P 1000 1 1000000
 *
 *  cache-thrash-hoard 1 1000 1 1000000
 *  cache-thrash-hoard P 1000 1 1000000
 *
 *  The ideal is a P-fold speedup.
*/

#include <iostream>
#include <stdlib.h>

using namespace std;

#include "cpuinfo.h"
#include "fred.h"
#include "timer.h"

#include "../common/clock.h"
#include "../common/worker.h"

int main(int argc, char *argv[]) {
  int nthreads = 1;
  int iterations = 1000;
  int objSize = 1;
  int repetitions = 1000000;
  uint64_t ns;
  MyTimer tmr;

  if (argc > 4) {
    nthreads = atoi(argv[1]);
    iterations = atoi(argv[2]);
    objSize = atoi(argv[3]);
    repetitions = atoi(argv[4]);
  }

  printf("Running cache-thrash for %d threads, %d iterations,"
	 " %d repetitions and %d objSize...  ",
	 nthreads, iterations, repetitions, objSize);

  HL::Fred *threads = new HL::Fred[nthreads];
  HL::Fred::setConcurrency(HL::CPUInfo::getNumProcessors());

  int i;

  // HL::Timer t;
  // t.start();
  ns_start(tmr);

  workerArg *w = new workerArg[nthreads];

  for (i = 0; i < nthreads; i++) {
    w[i] = workerArg(objSize, repetitions / nthreads, iterations);
    threads[i].create(&worker, (void *)&w[i]);
  }
  for (i = 0; i < nthreads; i++) {
    threads[i].join();
  }
  // t.stop();
  ns = ns_stop(tmr);
  
  delete[] threads;
  delete[] w;

  //cout << "Time elapsed = " << (double)t << " seconds." << endl;
  printf("cache-thrash time elapsed = %" PRIu64 " ~ns\n", ns);
}
