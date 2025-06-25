cmake_minimum_required(VERSION 3.20)
project(ems_env)

add_library(ems_env main.c)

install(TARGETS ems_env DESTINATION ${CMAKE_INSTALL_PREFIX})