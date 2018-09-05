cmake_minimum_required(VERSION 3.0)

project(ximiner CXX)
# Use C++11 features.
set (REQ_CPP11_FEATURES  cxx_strong_enums cxx_auto_type)

set(CMAKE_CXX_STANDARD 11) # C++11...
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wno-everything -msse4.2 ")

#Add header directy
include_directories("inc/")

add_executable(xim src/main.cpp )

