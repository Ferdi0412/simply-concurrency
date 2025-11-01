cmake_minimum_required(VERSION 3.12) 
project(SimplyConcurrency
    VERSION 0.0.0
    DESCRIPTION "Cross-platform threading"
    LANGUAGES CXX
)

add_library(Concurrency INTERFACE)
target_include_directories(Concurrency INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Require C++ 17
# target_compile_features(Concurrency INTERFACE cxx_std_17)

add_library(simply::Concurrency ALIAS Concurrency)

if(NOT WIN32)
    target_link_libraries(Concurrency INTERFACE pthread)
endif()

option(SIMPLY_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})

if(SIMPLY_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()