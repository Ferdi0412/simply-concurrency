cmake_minimum_required(VERSION 3.14)

# Following https://google.github.io/googletest/quickstart-cmake.html
include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

if(WIN32)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

FetchContent_MakeAvailable(googletest)

enable_testing()

# Function to add each test file for a specific C++ version
function(add_test name cxx_standard)
    set(test_name ${name}_test_cpp${cxx_standard})
    add_executable(${test_name} ${name}.cpp)
    target_link_libraries(${test_name} PRIVATE 
        Concurrency
        GTest::gtest_main
    )
    set_target_properties(${test_name} PROPERTIES
        CXX_STANDARD ${cxx_standard}
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )
    include(GoogleTest)
    gtest_discover_tests(${test_name})
endfunction()

# Iterate over each specific C++ version to build test for it
set(CXX_STANDARDS 17 20)
foreach(cxx_std ${CXX_STANDARDS})
    add_test(01_basics ${cxx_std})
endforeach()
