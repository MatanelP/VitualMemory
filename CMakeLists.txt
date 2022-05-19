cmake_minimum_required(VERSION 3.21)
project(Ex4)

set(CMAKE_CXX_STANDARD 14)

add_executable(Ex4 main.cpp MemoryConstants.h PhysicalMemory.cpp PhysicalMemory.h SimpleTest.cpp VirtualMemory.h VirtualMemory.cpp)
