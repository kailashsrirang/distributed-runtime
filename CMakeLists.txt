cmake_minimum_required(VERSION 3.20)

project(distributed_runtime VERSION 0.1.0 LANGUAGES CXX)

add_executable(runtime_demo src/main.cpp)

target_compile_features(runtime_demo PRIVATE cxx_std_20)

set_target_properties(runtime_demo PROPERTIES
    CXX_EXTENSIONS OFF
)

target_compile_options(runtime_demo PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
)