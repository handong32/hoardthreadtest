//          Copyright Boston University SESA Group 2013 - 2016.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <math.h>
#include <stdlib.h>
#include <chrono>
#include <signal.h>
#include <string>
#include <thread>
#include <iostream>

#include "../common/hoardEbb.h"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <ebbrt/Clock.h>
#include <ebbrt/Context.h>
#include <ebbrt/ContextActivation.h>
#include <ebbrt/GlobalIdMap.h>
#include <ebbrt/NodeAllocator.h>
#include <ebbrt/Runtime.h>
#include <ebbrt/StaticIds.h>

namespace po = boost::program_options;

using namespace ebbrt;

int main(int argc, char** argv) {
  auto bindir = boost::filesystem::system_complete(argv[0]).parent_path() /
                "/bm/hoardtest.elf32";

  ebbrt::Runtime runtime;
  ebbrt::Context c(runtime);
  ebbrt::ContextActivation activation(c);

  int numNodes, numThreads;
  try 
  {
      po::options_description desc("Program Options");
      desc.add_options()
	  ("help,h", "Print usage messages") 
	  ("numNodes", po::value<int>(&numNodes)->default_value(1), "Number of BM nodes to launch")
	  ("numThreads", po::value<int>(&numThreads)->default_value(1), "Number of threads to launch on each BM node")
	  ;
      po::variables_map vm;
      
      try 
      {
	  po::store(po::parse_command_line(argc, argv, desc), vm);  // can throw
	  
	  if (vm.count("help")) {
	      std::cout << "Application for EbbRT hoard code, e.g. Parameters: <number-of-threads> <iterations> <num-objects> <work-interval> <object-size>\n % hoardtest P 1000 10000 0 8" << std::endl << desc << std::endl;
	      return EXIT_SUCCESS;
	  }
	  
	  po::notify(vm);
      } 
      catch (po::error& e) 
      {
	  std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
	  std::cerr << desc << std::endl;
	  return EXIT_FAILURE;
      }
  } 
  catch (std::exception& e) 
  {
      std::cerr << "Unhandled exception while parsing arguments:  " << e.what()
		<< ", application will now exit" << std::endl;
      return EXIT_FAILURE;
  }
  
  auto hoard = hoardEbb::Create();
  hoard->setNumNodes(numNodes);
  hoard->setNumThreads(numThreads);

  for(int i = 0; i < numNodes; i++) {
  auto node_desc = node_allocator->AllocateNode(bindir.string());
  node_desc.NetworkId().Then(
      [hoard, &c](Future<Messenger::NetworkId> f) {
	  auto nid = f.Get();
	  hoard->addNid(nid);
      });
  }
  
  hoard->waitNodes().Then(
      [hoard](ebbrt::Future<void> f) {
	  f.Get();
	  std::cout << "all nodes initialized" << std::endl;
	  ebbrt::event_manager->Spawn([hoard]() {
		  hoard->Run();
	  });
      });

  hoard->waitRecv().Then([&c](ebbrt::Future<void> f) {
	      f.Get();
	      c.io_service_.stop();
     });

      c.Run();
      
  /*boost::asio::signal_set sig(c.io_service_, SIGINT);
  {
    ebbrt::ContextActivation activation(c);

    // ensure clean quit on ctrl-c
    sig.async_wait([&c](const boost::system::error_code& ec,
                        int signal_number) { c.io_service_.stop(); });
    Printer::Init().Then([bindir](ebbrt::Future<void> f) {
      f.Get();
      ebbrt::node_allocator->AllocateNode(bindir.string());
    });
  }
  c.Run();*/

  return 0;
}
