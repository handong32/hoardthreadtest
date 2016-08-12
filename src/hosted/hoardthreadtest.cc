//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

/*#include <signal.h>

#include <boost/filesystem.hpp>

#include <ebbrt/Context.h>
#include <ebbrt/ContextActivation.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/NodeAllocator.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/StaticIds.h>

//#include "Printer.h"

int main(int argc, char** argv) {
  auto bindir = boost::filesystem::system_complete(argv[0]).parent_path() /
                "/bm/hoardthreadtest.elf32";

  ebbrt::Runtime runtime;
  ebbrt::Context c(runtime);
  
  boost::asio::signal_set sig(c.io_service_, SIGINT);
  {
    ebbrt::ContextActivation activation(c);

    // ensure clean quit on ctrl-c
    sig.async_wait([&c](const boost::system::error_code& ec,
                        int signal_number) { c.io_service_.stop(); });
    Printer::Init().Then([bindir](ebbrt::Future<void> f) {
      f.Get();
      ebbrt::node_allocator->AppendArgs("niterations=50");
      ebbrt::node_allocator->AppendArgs("nobjects=30000");
      ebbrt::node_allocator->AppendArgs("nthreads=2");
      ebbrt::node_allocator->AppendArgs("work=0");
      ebbrt::node_allocator->AppendArgs("objSize=1");
      ebbrt::node_allocator->AllocateNode(bindir.string());
    });
  }
  c.Run();

  ebbrt::ContextActivation activation(c);

  Printer::Init().Then([bindir](ebbrt::Future<void> f) {
    f.Get();
    ebbrt::node_allocator->AppendArgs("abc=1");
    ebbrt::node_allocator->AllocateNode(bindir.string());
  });

  Printer::Wait().Then([&c](ebbrt::Future<void> f) {
    f.Get();
    c.io_service_.stop();
  });

  c.Run();

  return 0;
}
*/

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
#include <inttypes.h>

using namespace std;
using namespace std::chrono;

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <ebbrt/Clock.h>
typedef ebbrt::clock::HighResTimer MyTimer;
static inline void ns_start(MyTimer &t) { t.tick(); }
static inline uint64_t ns_stop(MyTimer &t) { return t.tock().count(); }


int niterations = 50;	// Default number of iterations.
int nobjects = 30000;  // Default number of objects.
int nthreads = 1;	// Default number of threads.
int work = 0;		// Default number of loop iterations.
int objSize = 1;


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
}

int main (int argc, char * argv[])
{
  thread ** threads;
  uint64_t ns;
  MyTimer tmr;
  
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

  //high_resolution_clock t;
  //auto start = t.now();
  
  ns_start(tmr);

  int i;
  for (i = 0; i < nthreads; i++) {
    threads[i] = new thread(worker);
  }

  for (i = 0; i < nthreads; i++) {
    threads[i]->join();
  }

  ns = ns_stop(tmr);
  
  //auto stop = t.now();
  //auto elapsed = duration_cast<duration<double>>(stop - start);
  std::printf("hoardthreadtest time elapsed = %" PRIu64 " ~ns\n", ns);
  //cout << "Time elapsed = " << elapsed.count() << endl;

  delete [] threads;

  return 0;
}
