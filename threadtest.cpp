///-*-C++-*-//////////////////////////////////////////////////////////////////
//
// Hoard: A Fast, Scalable, and Memory-Efficient Allocator
//        for Shared-Memory Multiprocessors
// Contact author: Emery Berger, http://www.cs.utexas.edu/users/emery
//
// Copyright (c) 1998-2000, The University of Texas at Austin.
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
 * @file threadtest.cpp
 *
 * This program does nothing but generate a number of kernel threads
 * that allocate and free memory, with a variable
 * amount of "work" (i.e. cycle wasting) in between.
*/

#ifndef _REENTRANT
#define _REENTRANT
#endif

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdint>

using namespace std;
using namespace std::chrono;

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "pcm_lite/Perf.h"
#include "pcm_lite/Rapl.h"


int niterations = 50;	// Default number of iterations.
int nobjects = 30000;  // Default number of objects.
int nthreads = 1;	// Default number of threads.
int work = 0;		// Default number of loop iterations.
int objSize = 1;

std::atomic<uint64_t> ninstructions(0);
std::atomic<uint64_t> ncycles(0);
std::atomic<uint64_t> nllc_miss(0);
std::atomic<uint64_t> nllc_ref(0);
std::atomic<uint64_t> joules(0);

class Foo {
public:
  Foo (void)
    : x (14),
      y (29)
    {}

  int x;
  int y;
};


void worker ()
{
  int i, j;
  Foo ** a;
  size_t mcore;
  rapl::RaplCounter rp;
  perf::PerfCounter nins;
  perf::PerfCounter ncyc;
  perf::PerfCounter nllcm;
  perf::PerfCounter nllcr;
  
  mcore = sched_getcpu();
  nins = perf::PerfCounter(perf::PerfEvent::fixed_instructions);
  ncyc = perf::PerfCounter(perf::PerfEvent::fixed_cycles);
  nllcm = perf::PerfCounter(perf::PerfEvent::llc_misses);
  nllcr = perf::PerfCounter(perf::PerfEvent::llc_references);
  
  if(mcore == 0 || mcore == 1) {
    rp = rapl::RaplCounter();
    rp.Start();
  }
  
  nins.Start();
  ncyc.Start();
  nllcm.Start();
  nllcr.Start();
  
  a = new Foo * [nobjects / nthreads];

  for (j = 0; j < niterations; j++) {

    for (i = 0; i < (nobjects / nthreads); i ++) {
      a[i] = new Foo[objSize];
#if 1
      for (volatile int d = 0; d < work; d++) {
	volatile int f = 1;
	f = f + f;
	f = f * f;
	f = f + f;
	f = f * f;
      }
#endif
      assert (a[i]);
    }
    
    for (i = 0; i < (nobjects / nthreads); i ++) {
      delete[] a[i];
#if 1
      for (volatile int d = 0; d < work; d++) {
	volatile int f = 1;
	f = f + f;
	f = f * f;
	f = f + f;
	f = f * f;
      }
#endif
    }
  }

  delete [] a;
  nins.Stop();
  ncyc.Stop();
  nllcm.Stop();
  nllcr.Stop();
  if(mcore == 0 || mcore == 1) {    
      rp.Stop();
      joules.fetch_add(rp.Read(), std::memory_order_relaxed);
      rp.Clear();
  }

  ninstructions.fetch_add(nins.Read(), std::memory_order_relaxed);
  ncycles.fetch_add(ncyc.Read(), std::memory_order_relaxed);
  nllc_ref.fetch_add(nllcr.Read(), std::memory_order_relaxed);
  nllc_miss.fetch_add(nllcm.Read(), std::memory_order_relaxed);
  
  nins.Clear();
  ncyc.Clear();
  nllcm.Clear();
  nllcr.Clear();  
}

int main (int argc, char * argv[])
{
  thread ** threads;
 
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

  printf ("Running threadtest for %d threads, %d iterations, %d objects, %d work and %d objSize...\n", nthreads, niterations, nobjects, work, objSize);

  threads = new thread*[nthreads];

  high_resolution_clock t;
  auto start = t.now();

  int i;
  for (i = 0; i < nthreads; i++) {
    threads[i] = new thread(worker);
  }

  for (i = 0; i < nthreads; i++) {
    threads[i]->join();
  }

  auto stop = t.now();
  auto elapsed = duration_cast<duration<double>>(stop - start);
  cout << "Time Instructions Cycles LLC_REF LLC_MISS Joules" << endl;
  cout << elapsed.count() << " "
       << ninstructions << " "
       << ncycles << " "
       << nllc_ref << " "
       << nllc_miss << " "
       << joules << endl;
  
  delete [] threads;

  return 0;
}
