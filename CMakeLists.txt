cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(txtrhashgen)

add_executable(txtrhashgen
  xxhash.h xxhash.c
  main.c)
