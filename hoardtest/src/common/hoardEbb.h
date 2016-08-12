#ifndef APPS_HOARD_EBB_H_
#define APPS_HOARD_EBB_H_

#include <ebbrt/Clock.h>
#include <ebbrt/EbbAllocator.h>
#include <ebbrt/Future.h>
#include <ebbrt/Message.h>
#include <ebbrt/IOBuf.h>
#include <ebbrt/UniqueIOBuf.h>
#include <ebbrt/StaticIOBuf.h>

class hoardEbb : public ebbrt::Messagable<hoardEbb> {

private:
    std::vector<ebbrt::Messenger::NetworkId> _nids;
    ebbrt::EventManager::EventContext* _emec {nullptr};
    ebbrt::Promise<void> nodesinit;
    ebbrt::Promise<void> mypromise;
    int _numNodes;
    int _numThreads;
    int _recv;
    
public:
    void setNumNodes(int n) { _numNodes = n; }
    void setNumThreads(int n) { _numThreads = n; }
  
    void addNid(ebbrt::Messenger::NetworkId nid);
    ebbrt::Future<void> waitNodes();
    ebbrt::Future<void> waitRecv();

    static ebbrt::EbbRef<hoardEbb> Create(ebbrt::EbbId id = ebbrt::ebb_allocator->Allocate());
    static hoardEbb& HandleFault(ebbrt::EbbId id);
    hoardEbb(ebbrt::EbbId ebbid);
    void Run();
    void ReceiveMessage(ebbrt::Messenger::NetworkId nid,
			std::unique_ptr<ebbrt::IOBuf>&& buffer);
};

typedef ebbrt::clock::HighResTimer MyTimer;
static inline void ns_start(MyTimer &t) { t.tick(); }
static inline uint64_t ns_stop(MyTimer &t) { return t.tock().count(); }

#endif
