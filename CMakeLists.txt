cmake_minimum_required(VERSION 2.8.12)

project(ximiner CXX)
# Use C++11 features.
set (REQ_CPP11_FEATURES  cxx_strong_enums cxx_auto_type)

set(CMAKE_CXX_STANDARD 11) # C++11...
set(CMAKE_CXX_STANDARD_REQUIRED ON)


if(UNIX AND NOT APPLE)
  set(LINUX TRUE)
endif()

#only enable parallel/algorithm(based on openMP) for the gnu parallel sort on linux.
if(LINUX)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3  -fopenmp -msse4.2 -ffast-math -funroll-loops -march=core-avx2")
else()
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wno-everything -msse4.2 ")
endif()

#Add header directy
include_directories("inc/")


add_executable(xim src/main.cpp )
add_executable(mwe src/segfaultmwe.cpp)
add_executable(hdrTest src/hdrTest.cpp)


