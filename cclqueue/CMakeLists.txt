cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0048 NEW)

project(cclqueue LANGUAGES CXX)

add_compile_options(-O2 -std=c++11)

include_directories(${cclqueue_SOURCE_DIR})

aux_source_directory(. cclqueue_srcs)

add_library(cclqueue STATIC ${cclqueue_srcs})
