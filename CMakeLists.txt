cmake_minimum_required(VERSION 3.14)

project(tcpserverdemo LANGUAGES CXX)

add_subdirectory(${CMAKE_SOURCE_DIR}/cclqueue)

file(GLOB_RECURSE
    server_srcs
    tcpserverdemo.cc
    main.cc)

add_executable(tcpserver ${server_srcs})
target_link_libraries(tcpserver cclqueue pthread)

add_executable(client clientexample.cc)
target_link_libraries(client pthread)
