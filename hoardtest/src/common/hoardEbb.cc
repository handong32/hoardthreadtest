#include "hoardEbb.h"

#include <math.h>
#include <stdlib.h>
#include <thread>
#include <inttypes.h>

#include <ebbrt/EbbRef.h>
#include <ebbrt/Future.h>
#include <ebbrt/IOBuf.h>
#include <ebbrt/LocalIdMap.h>
#include <ebbrt/Message.h>
#include <ebbrt/SharedEbb.h>
#include <ebbrt/SpinBarrier.h>
#include <ebbrt/StaticIOBuf.h>
#include <ebbrt/UniqueIOBuf.h>

#include <time.h>
#include <unistd.h>

EBBRT_PUBLISH_TYPE(, hoardEbb);

using namespace ebbrt;
using namespace std;

#ifdef __EBBRT_BM__
#define PRINTF ebbrt::kprintf
static size_t indexToCPU(size_t i) { return i; }
#else
#define PRINTF std::printf
#endif

int niterations = 50; // Default number of iterations.
int nobjects = 30000; // Default number of objects.
int nthreads = 1;     // Default number of threads.
int work = 0;         // Default number of loop iterations.
int objSize = 1;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#pragma GCC diagnostic ignored "-Wreturn-type"

class Foo {
public:
  Foo(void) : x(14), y(29) {}
  int x;
  int y;
};

void worker(void) {
  int i, j;
  Foo **a;
  a = new Foo *[nobjects / nthreads];

  for (j = 0; j < niterations; j++) {

    for (i = 0; i < (nobjects / nthreads); i++) {
      a[i] = new Foo[objSize];
      for (volatile int d = 0; d < work; d++) {
        volatile int f = 1;
        f = f + f;
        f = f * f;
        f = f + f;
        f = f * f;
      }
      assert(a[i]);
    }

    for (i = 0; i < (nobjects / nthreads); i++) {
      delete[] a[i];
      for (volatile int d = 0; d < work; d++) {
        volatile int f = 1;
        f = f + f;
        f = f * f;
        f = f + f;
        f = f * f;
      }
    }
  }

  delete[] a;
}

EbbRef<hoardEbb> hoardEbb::Create(EbbId id) { return EbbRef<hoardEbb>(id); }

// This Ebb is implemented with one representative per machine
hoardEbb &hoardEbb::HandleFault(EbbId id) {
  {
    // First we check if the representative is in the LocalIdMap (using a
    // read-lock)
    LocalIdMap::ConstAccessor accessor;
    auto found = local_id_map->Find(accessor, id);
    if (found) {
      auto &rep = *boost::any_cast<hoardEbb *>(accessor->second);
      EbbRef<hoardEbb>::CacheRef(id, rep);
      return rep;
    }
  }

  hoardEbb *rep;
  {
    // Try to insert an entry into the LocalIdMap while holding an exclusive
    // (write) lock
    LocalIdMap::Accessor accessor;
    auto created = local_id_map->Insert(accessor, id);
    if (unlikely(!created)) {
      // We raced with another writer, use the rep it created and return
      rep = boost::any_cast<hoardEbb *>(accessor->second);
    } else {
      // Create a new rep and insert it into the LocalIdMap
      rep = new hoardEbb(id);
      accessor->second = rep;
    }
  }
  // Cache the reference to the rep in the local translation table
  EbbRef<hoardEbb>::CacheRef(id, *rep);
  return *rep;
}

hoardEbb::hoardEbb(EbbId ebbid) : Messagable<hoardEbb>(ebbid) {
  _nids.clear();
  _numNodes = 0;
  _numThreads = 0;
  _recv = 0;
}

void hoardEbb::addNid(ebbrt::Messenger::NetworkId nid) {
  _nids.push_back(nid);
  if ((int)_nids.size() == _numNodes) {
    nodesinit.SetValue();
  }
}

ebbrt::Future<void> hoardEbb::waitNodes() {
  return std::move(nodesinit.GetFuture());
}

ebbrt::Future<void> hoardEbb::waitRecv() {
  return std::move(mypromise.GetFuture());
}

void hoardEbb::ReceiveMessage(Messenger::NetworkId nid,
                              std::unique_ptr<IOBuf> &&buffer) {
  auto dp = buffer->GetDataPointer();
  auto ret = dp.Get<uint64_t>();
  // PRINTF("Received %d bytes, %d\n", (int)buffer->ComputeChainDataLength(),
  // ret);

  uint64_t ns;
  MyTimer tmr;
  int i;
  
  PRINTF("Running threadtest for %d threads, %d iterations,"
         " %d objects, %d work and %d objSize...\n",
         nthreads, niterations, nobjects, work, objSize);

#ifdef __EBBRT_BM__

  size_t ncpus = nthreads; // ebbrt::Cpu::Count();
  static ebbrt::SpinBarrier bar(ncpus);
  ebbrt::EventManager::EventContext context;
  std::atomic<size_t> count(0);
  size_t theCpu = ebbrt::Cpu::GetMine();

  ns_start(tmr);
  for (size_t i = 0; i < ncpus; i++) {
    // spawn jobs on each core using SpawnRemote
    ebbrt::event_manager->SpawnRemote(
        [theCpu, ncpus, &count, &context, i]() {
          ebbrt::kprintf("running core %d of %d\n", (int)i, (int)ncpus);
          worker();
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

  auto retbuf = MakeUniqueIOBuf(1 * sizeof(uint64_t));
  auto retdp = retbuf->GetMutDataPointer();
  retdp.Get<uint64_t>() = ns;
  SendMessage(nid, std::move(retbuf));
#else
  PRINTF("hoardthreadtest baremetal time elapsed = %" PRIu64 " ~ns\n", ret);
  
  thread ** threads;
  threads = new thread*[nthreads];
  
  ns_start(tmr);
  for (i = 0; i < nthreads; i++) {
    threads[i] = new thread(worker);
  }

  for (i = 0; i < nthreads; i++) {
    threads[i]->join();
  }
  ns = ns_stop(tmr);
  
  PRINTF("hoardthreadtest frontend time elapsed = %" PRIu64 " ~ns\n", ns);
  delete [] threads;

  _recv++;
  if (_recv == _numNodes) {
    _recv = 0;
    mypromise.SetValue();
  }
#endif
}

void hoardEbb::Run() {
  for (int i = 0; i < (int)_nids.size(); i++) {
    auto buf = MakeUniqueIOBuf(1 * sizeof(uint64_t));
    auto dp = buf->GetMutDataPointer();
    dp.Get<uint64_t>() = 0;
    SendMessage(_nids[i], std::move(buf));
  }
}

#pragma GCC diagnostic pop
