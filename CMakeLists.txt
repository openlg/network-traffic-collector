cmake_minimum_required(VERSION 3.13)
project(network_traffic_collector C)

set(CMAKE_C_STANDARD 11)

include(FindPCAP.cmake)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

add_executable(ntc ntc.h ntc.c options.h options.c util.c addrs_ioctl.c filter.c filter.h log.c log.h)
target_link_libraries(ntc ${PCAP_LIBRARY} CURL::libcurl ${CMAKE_THREAD_LIBS_INIT})