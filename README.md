# Simply Concurrency
[![C++17](https://img.shields.io/badge/C++-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Platform](https://img.shields.io/badge/platform-Windows-lightgrey)](https://docs.microsoft.com/en-us/windows/)
![Status](https://img.shields.io/badge/status-alpha-orange)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Simply Concurrency provides a drop-in replacement for `std::thread` with additional features (and minor changes) for flexibility and ease of use.

> **Status:** Alpha - Only supported for Windows, API is subject to minor changes

**Key Features**
- Thread priority control
- Drop-in replacement for `std::thread` (only change is **join-on-destructor** instead of **terminate-on-destructor**)
- Single-header, header-only - no linking required
- C++ 17/20 compatible

## Why Simply Concurrency?
`std::thread` and `std::jthread` make creating threads easy, but lack control over execution options like priority.

**Example:** Want a background thread that doesn't interfere with UI responsiveness?
```c++
// Standard library - no priority control
std::thread t(worker);

simply::Thread t({
    .priority = simply::Thread::Priority::LOW
}, worker)
```

## Quick Start
### Installation
Simply Concurrency is header-only. Download `concurrency.h` and include it:
```c++
#include "concurrency.h"
```

Alternatively if using **CMake**, **FetchContent** can be used. **CMakeLists.txt:**
```txt
FetchContent_Declare(
    SimplyCOncurrency
    GIT_REPOSITORY https://github.com/Ferdi0412/simply-concurrency.git
    GIT_TAG main
)

FetchContent_MakeAvailable(SimplyConcurrency)

add_executable(main main.cpp)
target_link_libraries(my_program PRIVATE simply::Concurrency)
```

**main.cpp:**
```c++
#include <simply/concurrency.h>
```

### Compilation
No special flags needed, just ensure C++17 or later:
```sh
g++ -std=c++17 program.cpp -o program
cl /std:c++17 program.cpp
```

### Basic Usage
```c++
#include "concurrency.h"
#include <iostream>

void print_msg(std::string msg) {
    simply::this_thread::sleep(1000); // 1 second
    std::cout << msg << std::endl;
}

int main() {
    simply::Thread t1(print_msg, "Hello world!");
    
    simply::Thread t2({
        .priority = simply::Thread::Priority::HIGH
    }, print_msg, "Hello world!");

    // Block here for threads to complete...
}
```

### Comparison with Standard Library
```c++
// If file downloaded directly:
#include "concurrency.h"

// If using FetchContent
#include <simply/concurrency.h>

#include <thread>
#include <chrono>
#include <iostream>

void worker(bool use_std, std::string msg) {
    // Both of the following have the same behaviour
    if ( use_std )
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    else
        simply::this_thread::sleep(1000);
    std::cout << msg << std::endl;
}

// =====================================================================
// Example 1 >> Destructor Behaviour
// =====================================================================
// The only difference in the "standard" behaviours is how the destructor
// handles an un-joined thread

// The standard library terminates the program
void std_1() {
    std::thread t1(worker, true, "Hello world!");

    // Kill program here because no join...
}

// C++20 added `jthread` to join instead of terminating instead
void std_j1() {
    std::jthread t1(worker, true, "Hello world!");
}

// The Simply library does the same as `jthread`, and works for C++17 too
void simply_1() {
    simply::Thread t1(worker, false, "Hello world!");

    // Wait for worker to finish here...
}

// =====================================================================
// Example 2 >> Detach Behaviour
// =====================================================================
// To neither wait, nor terminate, both have the same detach behaviour

void std_2() {
    std::thread t1(worker, true, "Hello world!");
    t1.detach();
}

void simply_2() {
    std::thread t1(worker, false, "Hello world!");
    t1.detach();
}

// =====================================================================
// Example 3 >> Setting Priority
// =====================================================================
// With the standard library implementation you must be familiar with
// the operating system's APIs for controlling threads, and manually
// handle any relevant exceptions (not done in std_3 example)
//
// Both of the following are equivalent

#if _WIN32
    #include <windows.h> // Needed std_3
#else
    // ...
#endif
void std_3() {
    std::thread t1(worker, true, "Hello world!");
    
    #ifdef _WIN32
        SuspendThread(t1.native_handle());
        SetThreadPriority(t1.native_handle(), THREAD_PRIORITY_HIGH);
        ResumeThread(t1.native_handle());
    #else
        // ...
    #endif

    t1.join();
}

void simply_3() {
    simply::Thread t2({
        .priority = simply::Thread::Priority::HIGH
    }, worker, false, "Hello world!");
}

// =====================================================================
// Example 4 >> Thread Safety
// =====================================================================
// Both implementations will let the main program terminate for any
// uncaught exceptions. There is a planned class named FutureThread 
// which will be "safer" and catch any exceptions.

#include <exception>
void unsafe_worker() {
    throw std::runtime_error("Some message...");
}

void std_4() {
    std::thread t1(unsafe_worker);
    t1.join(); // terminates
}

void simply_4() {
    simply::Thread t1(unsafe_worker);
    t1.join(); // terminates
}

// =====================================================================
// Example 5 >> Pass by reference
// =====================================================================
// Both implementations need to explicitly pass references using `std::ref`

void make_5(double& val) {
    val = 5;
}

void std_5() {
    double global_val = 0;

    std::thread t1(make_5, global_val);
    t1.join(); // global_val is still 0

    std::thread t2(make_5, std::ref(global_val));
    t2.join(); // global_val is now 5
}

void simply_5() {
    double global_val = 0;

    simply::Thread t1(make_5, global_val);
    t1.join(); // global_val is still 0

    simply::Thread t2(make_5, std::ref(global_val));
    t2.join(); // global_val is now 5
}

// =====================================================================
// Example 6 >> Timeout 
// =====================================================================
// Only simply allows join to timeout

void simply_6() {
    simply::Thread t1(worker, "Hello world!");
    if ( t1.join(100) )
        std::cout << "Joined within 100 ms!" << std::endl;
    else
        std::cout << "Could not join within 100 ms! Must join again later, or detach..." << std::endl;
}
```

**Summary:**
| Feature | `std` | `simply` |
| ------: | :---: | :------: |
| Basic threading | YES | YES |
| Thread Priority | NO | YES |
| Destructor Behaviour | `std::terminate` if joinable (ends program) | `join` if joinable (may block program) |
| Timeout on join | NO | YES |
| Cross-platform | YES | PLANNED |
| Zero overhead | YES | YES |

Both implementations provide (near) **zero overhead**, as they are direct wrappers around the OS-specific APIs.

I am currently developing on Windows, will soon implement `Thread` for Linux, I don't have access to macOS device for testing on currently.

## Requirements
- **C++ Standard:** C++17 or later
- **Dependencies:** 
    - Windows SDK (automatically included in most compilers on Windows)

## API Reference
### `class simply::Thread`

**Constructors**
```c++
Thread(); // Empty thread object
Thread(F&& f, Args&&... args); // Create and start thread with any number of arguments
Thread(Options opt, F&& f, Args&&... args); // Create with options
```

**Key Operations**
| Method | Description |
| -----: | :---------- |
| `void join()` | Block until thread completes |
| `bool join(size_t ms)` | Wait with timeout, returns false if timed out |
| `void detach()` | Detach thread for independent execution |
| `bool joinable() const` | Check if thread can be joined |
| `id get_id() const` | Get a unique (and hashable) identifier |
| `static unsigned int hardware_concurrency()` | Get number of system-supported hardware threads |

**Priority Levels**
```c++
Thread::Priority::LOWEST
Thread::Priority::LOW
Thread::Priority::NORMAL
Thread::Priority::HIGH
Thread::Priority::HIGHEST
Thread::Priority::TIME_CRITICAL
```

**Planned**
| Method | Description |
| -----: | :---------- |
| `static bool time_critical_available()` | Check if the program has system rights to start `TIME_CRITICAL` threads |

### `namespace simply::this_thread`
| Method | Description |
| -----: | :---------- |
| `get_id()` | Get ID for current thread |
| `yield()` | Let operating system yield to another thread |
| `sleep(size_t ms)` | Sleep for (minimum number of) milliseconds |

## Roadmap
- [ ] Linux support (pthread implementations)
- [x] `std::stop_token` integration for C++20 (like in `std::jthread`)
- [ ] `simply::FutureThread` (inspired in part by `std::async`)
- [ ] If feedback suggests it, or I need it: CPU affinity and stack size control
- [x] GitHub CI/CD workflows 

## Issues
Please report issues on the [GitHub Issues](https://github.com/Ferdi0412/simply-concurrency/issues) page.

### Feedback and Contributing
I would also love some feedback on the API - changes to behaviours such as join-on-destructor, and further options to add.

Contributions are also very welcome! Feel free to submit a pull request.

The current naming and comment conventions may be a little unconventional, but they are what I find work best, but they are also open to revision.

## Author
Ferdinand Oliver M Tonby-Strandborg