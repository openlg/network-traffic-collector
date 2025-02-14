cmake_minimum_required(VERSION 3.13)
project(network_traffic_collector C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}")
set(CMAKE_STRIP "strip")

include(FindPCAP.cmake)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

add_executable(ntc ntc.h ntc.c options.h options.c
        util.h util.c addrs_ioctl.c
        filter.c filter.h
        log.c log.h
        signature.c signature.h
        server.c server.h)
target_link_libraries(ntc ${PCAP_LIBRARY} CURL::libcurl ${CMAKE_THREAD_LIBS_INIT} ${OPENSSL_LIBRARIES})

add_executable(ntc_test ntc.h
        options.h options.c test.c
        util.h util.c
        filter.h filter.c
        log.c log.h
        signature.c signature.h)
target_link_libraries(ntc_test ${OPENSSL_LIBRARIES})
enable_testing()
add_test(NAME ntc_test COMMAND ntc_test)