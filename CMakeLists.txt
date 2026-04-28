cmake_minimum_required(VERSION 3.20)
project(RayTracer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Release build is much faster for rendering
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(raytracer
    src/main.cpp
)

target_include_directories(raytracer PRIVATE src)
