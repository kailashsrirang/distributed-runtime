cmake_minimum_required(VERSION 3.20)

project(distributed_runtime VERSION 0.1.0 LANGUAGES CXX)

# Compiles src/statistics.cpp into a static library. run_timecore is like a module
add_library(runtime_core STATIC
    src/statistics.cpp
    src/dataset.cpp
    src/tcp.cpp
    src/protocol.cpp
    src/message.cpp
)
# where to find the header files for this library
target_include_directories(runtime_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(runtime_core PUBLIC cxx_std_20)

# compiles into executables 
add_executable(runtime_demo src/main.cpp)
add_executable(coordinator src/coordinator.cpp)
add_executable(worker src/worker.cpp)

#Connects the runtime_core library to the all executables, allowing it to use the functions and classes defined in runtime_core.
target_link_libraries(runtime_demo PRIVATE runtime_core)
target_link_libraries(coordinator PRIVATE runtime_core)
target_link_libraries(worker PRIVATE runtime_core)

#enable smake's built-in testing framework
include(CTest)

if(BUILD_TESTING)
    #compiles statistics_test.cpp into an executable 
    add_executable(statistics_test tests/statistics_test.cpp)
    #links it to runtime_core module
    target_link_libraries(statistics_test PRIVATE runtime_core)
    # register the test with CTest, so it can be run with ctest command
    add_test(NAME statistics COMMAND statistics_test)

    add_executable(dataset_test tests/dataset_test.cpp)
    target_link_libraries(dataset_test PRIVATE runtime_core)
    add_test(NAME dataset COMMAND dataset_test)

    add_executable(unique_fd_test tests/unique_fd_test.cpp)
    target_link_libraries(unique_fd_test PRIVATE runtime_core)
    add_test(NAME unique_fd COMMAND unique_fd_test)

    add_executable(protocol_test tests/protocol_test.cpp)
    target_link_libraries(protocol_test PRIVATE runtime_core)
    add_test(NAME protocol COMMAND protocol_test)

    add_executable(message_test tests/message_test.cpp)
    target_link_libraries(message_test PRIVATE runtime_core)
    add_test(NAME message COMMAND message_test)
    set_tests_properties(message PROPERTIES TIMEOUT 5)
    
endif()

set(project_targets runtime_core runtime_demo coordinator worker)

if(BUILD_TESTING)
    list(APPEND 
    project_targets
    statistics_test
    dataset_test
    unique_fd_test
    protocol_test
    )
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