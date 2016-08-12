cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
set(CMAKE_CXX_FLAGS_DEBUG          "-O0 -g3")
set(CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE        "-O4 -flto -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g3")

project("hoardtest-ebbrt" CXX)

set(HOSTED_SOURCES 
      src/hosted/hoardtest.cc
      src/common/hoardEbb.cc)

set(BAREMETAL_SOURCES 
      src/common/hoardEbb.cc)

# Baremetal  ========================================================

if( ${CMAKE_SYSTEM_NAME} STREQUAL "EbbRT")

  message(STATUS "Building baremetal image...")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14 -Wall -Werror -D__EBBRT_BM__")
  add_executable(hoardtest.elf ${BAREMETAL_SOURCES})
  add_custom_command(TARGET hoardtest.elf POST_BUILD 
    COMMAND objcopy -O elf32-i386 hoardtest.elf hoardtest.elf32 )

# Hosted  ===========================================================

elseif( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
  message(STATUS "Building Linux executable...")

  # TODO(jmcadden): remove source-dir requirement 
  if(DEFINED ENV{EBBRT_SRCDIR})
    list(APPEND CMAKE_MODULE_PATH 
      "$ENV{EBBRT_SRCDIR}/hosted/cmake/Modules")
  endif()

  find_package(EbbRT REQUIRED)
  find_package(Boost 1.53.0 REQUIRED COMPONENTS 
    filesystem system coroutine context program_options)
  find_package(Capnp REQUIRED)
  find_package(TBB REQUIRED)
  
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++14 -Wall -Werror -lpthread")
  include_directories(${EBBRT_INCLUDE_DIRS})
  add_executable(hoardtest ${HOSTED_SOURCES})
  target_link_libraries(hoardtest -Wl,--whole-archive ${EBBRT_LIBRARIES}
    -Wl,--no-whole-archive -lpthread
    ${CAPNP_LIBRARIES_LITE} ${CMAKE_THREAD_LIBS_INIT} 
    ${Boost_LIBRARIES} ${TBB_LIBRARIES} 
    )

else()
  message(FATAL_ERROR "System name unsupported: ${CMAKE_SYSTEM_NAME}")
endif()
