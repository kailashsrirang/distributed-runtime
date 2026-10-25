cmake_minimum_required(VERSION 3.20)

project(distributed_runtime VERSION 0.1.0 LANGUAGES CXX)

# Compiles src/statistics.cpp into a static library. run_timecore is like a module
add_library(runtime_core STATIC
    src/statistics.cpp
    src/dataset.cpp
)
# where to find the header files for this library
target_include_directories(runtime_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(runtime_core PUBLIC cxx_std_20)

# compiles src/main.cpp into an executable 
add_executable(runtime_demo
    src/main.cpp
)

#Connects the runtime_core library to the runtime_demo executable, allowing it to use the functions and classes defined in runtime_core.
target_link_libraries(runtime_demo PRIVATE runtime_core)

#enable smake's built-in testing framework
include(CTest)

if(BUILD_TESTING)
    #compiles statistics_test.cpp into an executable 
    add_executable(statistics_test
        tests/statistics_test.cpp
    )

    #links it to runtime_core module
    target_link_libraries(statistics_test PRIVATE runtime_core)

    # register the test with CTest, so it can be run with ctest command
    add_test(NAME statistics COMMAND statistics_test)

    add_executable(dataset_test
        tests/dataset_test.cpp
    )

    target_link_libraries(dataset_test PRIVATE runtime_core)

    add_test(NAME dataset COMMAND dataset_test)
endif()

set(project_targets runtime_core runtime_demo)

if(BUILD_TESTING)
    list(APPEND project_targets statistics_test dataset_test)
endif()

foreach(target IN LISTS project_targets)
    set_target_properties(${target} PROPERTIES
        CXX_EXTENSIONS OFF
    )

    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    )
endforeach()