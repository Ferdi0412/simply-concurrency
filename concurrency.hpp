/// concurrency.hpp
/// Lightweight threading library with priority control (currently Windows only)
///
/// Copyright (c) 2025 Ferdinand Oliver M Tonby-Strandborg
/// SPDX-License-Identifier: MIT
/// 
/// Version:       0.0.0-alpha
///
/// Requirements:  C++17 or later
///                Windows Vista or later
///
/// Distribution:  Single-header library - include this file
///
/// Documentation: Using a custom style for readability in header file.
///                Chosen over Doxygen as no documentation generation
///                is planned for immediate versions.
///
/// Repository:    https://github.com/Ferdi0412/simply-concurrency
/// 
/// Issues:        https://github.com/Ferdi0412/simply-concurrency/issues

/// ====================================================================
/// Introduction
/// ====================================================================
///   Brief
/// This header-only library provides threading utility with control over
/// thread priority, as an alternative to std::thread and std::jthread.
/// 
///   Classes
/// simply::Thread
///     This tries to provide a nearly drop-in replacement for 
///     `std::thread` or `std::jthread` with added control,
///     for example to set thread execution priority.
///
/// simply::FutureThread
///     This is a planned (not currently implemented) class to provide
///     something closer to `std::async`. Details to come...
///
///   Functions
/// simply::this_thread::get_id
///     To compare an instance of Thread/FutureThread with the current
///     thread of execution.
///
/// simply::this_thread::yield
///     To yield to other threads. What thread is yielded to is 
///     determined by the system.
///
/// simply::this_thread::sleep
///     To sleep for a minimum (and often almost exact)
///     number of milliseconds
///
/// Support for other operating systems will come later...
///
/// Documentation note - I have not used Doxygen style, simply because
/// I find that it often becomes so dense and hard to read.
#ifndef SIMPLY_CONCURRENCY_HPP_
#define SIMPLY_CONCURRENCY_HPP_

// Require C++ 17+
#if __cplusplus < 201703L // C++ < 17
    #error "Only available for C++ >= 17 due to std::optional and std::apply"
#endif

#ifndef SIMPLY_NODISCARD
#define SIMPLY_NODISCARD [[nodiscard]] // For C++ 17+
#endif

// Require Windows
#ifndef _WIN32
    #error "Only available for windows right now!"
#endif 

#include <string>
#include <optional>
#include <algorithm>
#include <memory>
#include <functional>
#include <system_error>

#include <windows.h>

///   simply
/// Everything from the simply library(s) will be wrapped in this namespace
namespace simply {

// =====================================================================
// Thread >> Declaration
// =====================================================================
///   Thread
/// This allows control over priority, with a similar interface to `std::jthread`
///
///   Behaviours
/// - `join` on destructor
///     This means that the destructor **will block**. If you do not want
///     this, detach the thread before its destructor is called.
/// - `terminate` on unhandled exception
///     If the callback/function throws any uncaught exceptions, the
///     entire program will terminate. To avoid this, you may use a 
///     simple `try { } catch (...) { }` in your function.
/// - `system_error` thrown by most operations in case of failure
///     For example if a thread could not be made, or an unavailable
///     priority was set, this is thrown.
///     This will also be thrown if the user makes a mistake, such as
///     `simply::Thread().join();` (joining a "NULL" thread)
/// - Single-ownership
///     This means that the copy construtor and assigner are deleted.
///     Because of this (and ironically), Thread is not thread-safe.
///     You may share it using `std::share_ptr`, but there are no safety
///     mechanisms that prevent a data race for example for 2 concurrent
///     calls to `join`, which could lead to undefined behaviour.
/// - (planned) `stop_token` will be automatically given to relevant functions
///     This would allow the class to "nicely" stop the thread of execution
///     on the `join` command.
///
///   Pass-by-reference
/// As in `std::thread` and `std::jthread`, the arguments passed to the
/// functions are copied, and so anything argument to be passed
/// by reference should make this explicit using `std::ref`, for example:
/// ```
/// auto set_5 = [](double& val){ val = 5; }
/// double val = 0;
///
/// Thread t1(set_5, val);
/// t1.join(); // "global" val is still 0
/// 
/// Thread t2(set_5, std::ref(val));
/// t2.join(); // "global" val becomes 5
/// ```
class Thread final {
public:
    ///   native_handle_type
    /// Alias for platform-specific thread handle
    ///
    /// Unless you are familiar with OS-specific APIs for threading,
    /// do not touch anything returning this.
    typedef HANDLE native_handle_type;

    ///   id
    /// Used to uniquely identify each thread
    ///
    /// Follows the conventions of `class std::thread::id`
    class id;
    
    ///   Options
    /// Thread startup options
    class Options;

    ///   Priority
    /// Provided to make a cross-platform abstraction, such that the same
    /// code can run on Windows, (and when later supported) Linux and macOS
    enum class Priority { LOWEST, LOW, NORMAL, HIGH, HIGHEST, TIME_CRITICAL };

public:
    ///   Empty Constructor - NULL Thread
    /// Creates a threadless/empty object
    ///
    /// It can be identified using either of:
    /// - `.joinable() == false`
    /// - `.get_id() == id()`
    Thread() noexcept;

    ///   Destructor
    /// If joinable, will **block** until the thread can join
    ~Thread();

    ///   No copying
    Thread(const Thread&) = delete;
    Thread& operator=(const Thread&) = delete;

    ///   Move Constructor
    Thread(Thread&& other) noexcept;

    ///   Move Assignment
    /// If this is joinable, will **block** and join before moving other
    Thread& operator=(Thread&& other);

    ///  swap
    /// Swap two instances of this object
    void swap(Thread& other) noexcept;

    ///    Constructor
    /// Create and immediately execute on the new thread
    ///
    ///   Params
    /// f Function to execute on thread
    ///     Any returns are ignored.
    ///     Any exceptions thrown by it will cause `std::terminate`
    /// args Any number of arguments to pass to f
    ///     Unless explicitly passed with `std::ref`, will be copied,
    ///     as required by implementations of `std::thread` and `std::jthread`
    template <class F, class... Args>
    Thread(F&& f, Args&&... args);

    ///    Constructor
    /// 
    ///   Params
    /// opt Thread options, namely priority
    template <class F, class... Args>
    Thread(Options opt, F&& f, Args&&... args);

    ///   joinable
    /// Check if the thread can join or detach
    ///
    /// A thread is joinable if it represents a different thread
    /// than the calling thread which is non-detached and non-joined
    ///
    /// Does **not** check whether the thread has terminated, 
    /// join may still block for any amount of time.
    SIMPLY_NODISCARD bool joinable() const noexcept;

    ///   get_id
    /// Get a unique identifier for the thread
    SIMPLY_NODISCARD id get_id() const noexcept;

    ///   native_handle {dangerous}
    /// Get the native handle for the thread, for manual control
    ///
    /// This is dangerous, for example if this handle is used to
    /// manually join/detach the thread, then operations such as join
    /// would throw `system_error`
    ///
    /// Throws `system_error` if this is a NULL-thread object
    SIMPLY_NODISCARD native_handle_type native_handle(); 

    ///   hardware_concurrency
    /// Get the number of hardware threads supported by the system
    ///
    /// Consider this a hint, and no value is available `0` is returned
    ///
    /// {note: Windows} If 64 **or more** threads available, this will
    ///                 return `64` - Implementation restriction
    SIMPLY_NODISCARD static unsigned int hardware_concurrency() noexcept;

    ///   join
    /// Block until thread finishes execution
    ///
    /// (planned) Will request a stop through any `std::stop_token` used.
    void join();

    ///   join
    /// Block a limited time for thread to finish execution
    ///
    /// Returns whether the thread successfully joined
    /// A return of `false` means the thread is still joinable.
    /// (planned) Will request a stop through any `std::stop_token` used.
    SIMPLY_NODISCARD bool join(size_t ms_timeout);

    ///   detach
    /// Detach thread so its execution is independent and uncontrolled
    void detach();

private:
    native_handle_type _handle;
};
}

namespace std {
///   std::swap
/// Specialization of `std::swap` for the Thread class
void swap(simply::Thread& x, simply::Thread& y) noexcept;
}

namespace simply {
// =====================================================================
// this_thread >> Declarations
// =====================================================================
///   this_thread
/// This namespace provides some simple controls for the current thread
namespace this_thread {
    ///   get_id
    /// Get a unique identifier for the current thread of execution
    Thread::id get_id() noexcept;

    ///   yield
    /// Yield to another thread of execution
    void yield() noexcept;

    ///   sleep
    /// Sleep for the specified number of milliseconds
    void sleep(size_t ms_sleep);
}

// =====================================================================
// Thread::Options >> Full Implementation
// =====================================================================
///   Options
/// See notes in declaration inside Thread
///
/// Feel free to suggest more features to add
/// 
/// Possible future options:
/// - Stack size
/// - CPU affinity
class Thread::Options final {
public:
    ///   priority
    /// Optionally set
    std::optional<Thread::Priority> priority;
};

// =====================================================================
// Thread::id >> Declaration
// =====================================================================
///   id 
/// See notes in declaration within Thread
class Thread::id final {
protected:
    friend Thread;
    
    ///   id
    /// Create an instance representing an actual thread 
    ///
    /// Only Thread should ever create this
    id(DWORD i) noexcept;
    
public:
    ///   id
    /// This represents the id of the current thread of execution
    ///
    /// Is equivalent to the return of `simply::this_thread::id()`
    id() noexcept;

    size_t hash_value() const noexcept;

    ///   Comparisons...
    // == needed for both C++ 17 and 20
    friend bool operator==(id lhs, id rhs) noexcept;


#if __cplusplus >= 202002L 
    /// As of C++ 20, this is the preferred implementation of comparisons
    friend std::strong_ordering operator<=>(id lhs, id rhs) noexcept;

#else
    // Before C++ 20, each comparison must be independently implemented
    friend bool operator!=(id lhs, id rhs) noexcept;
    friend bool operator<(id lhs, id rhs) noexcept;
    friend bool operator<=(id lhs, id rhs) noexcept;
    friend bool operator>(id lhs, id rhs) noexcept;
    friend bool operator>=(id lhs, id rhs) noexcept;
#endif

    /// Printing...
    template <class CharT, class Traits>
    friend std::basic_ostream<CharT, Traits>&
        operator<<(std::basic_ostream<CharT, Traits>& ost, id id);

private:
    DWORD _id;
};
}

namespace std {
/// Specialization to allow hashing of the Thread, for example in an
/// `std::unordered_map`
template <>
struct hash<simply::Thread::id>;
}



namespace simply {
// =====================================================================
// Thread::id >> Implementations
// =====================================================================
Thread::id::id() noexcept: _id(GetCurrentThreadId()) {}
Thread::id::id(DWORD i) noexcept: _id(i) {}

// =====================================================================
// this_thread >> Implementations
// =====================================================================
Thread::id this_thread::get_id() noexcept 
    { return Thread::id(); }

void this_thread::yield() noexcept 
    { SwitchToThread(); }
    
void this_thread::sleep(size_t ms_sleep) 
    { Sleep(ms_sleep); }

// =====================================================================
// Thread::id >> Implementations 
// =====================================================================
size_t Thread::id::hash_value() const noexcept {
    return std::hash<DWORD>{}(_id);
}


inline bool operator==(Thread::id lhs, Thread::id rhs) noexcept 
    { return lhs._id == rhs._id; }

#if __cplusplus >= 202002L // C++ >= 20
inline std::strong_ordering operator<=>(Thread::id lhs, Thread::id rhs) noexcept
    { return lhs._id <=> rhs._id; }

#else // C++ < 20
inline bool operator!=(Thread::id lhs, Thread::id rhs) noexcept 
    { return lhs._id != rhs._id; }

inline bool operator<(Thread::id lhs, Thread::id rhs) noexcept 
    { return lhs._id < rhs._id; }

inline bool operator<=(Thread::id lhs, Thread::id rhs) noexcept
    { return lhs._id <= rhs._id; }

inline bool operator>(Thread::id lhs, Thread::id rhs) noexcept 
    { return lhs._id > rhs._id; }

inline bool operator>=(Thread::id lhs, Thread::id rhs) noexcept 
    { return lhs._id >= rhs._id; }
#endif

template <class CharT, class Traits>
inline std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& ost, Thread::id id) {
    ost << std::to_string(id._id);
    return ost;
}
}

namespace std {
template <>
struct hash<simply::Thread::id> {
    size_t operator()(const simply::Thread::id& id) const noexcept {
        return id.hash_value();
    }
};
}

// =====================================================================
// Thread >> System-API Wrappers
// =====================================================================
namespace simply {
template <class T, size_t... I>
unsigned __stdcall _invoke(void* lparg) noexcept {
    const std::unique_ptr<T> argptr(static_cast<T*>(lparg));
    T& args = *argptr; // Had compiler issues without this...
    std::invoke(std::move(std::get<I>(args))...);
    return 0;
}

// This is necessary for the compiler to "generate" an implementation
// of templated `_invoke` with appropriate signature...
template <class T, std::size_t... I>
constexpr auto _invoke_gen(std::index_sequence<I...>) noexcept {
    return &_invoke<T, I...>;
}

// Used to cleanup a started thread due to startup issues
// ONLY USE IN _start - thread must be newly created and in SUSPENDED state
inline void _cleanup_suspended(HANDLE& handle) noexcept {
    TerminateThread(handle, 0);
    CloseHandle(handle);
    handle = nullptr;
}

// Use a handle in case of error after thread completed...
template <class F, class... Args>
void _start(HANDLE& handle, const Thread::Options& opt, F&& f, Args&&... args) {
    using T = std::tuple<std::decay_t<F>, std::decay_t<Args>...>;

    static_assert(std::is_invocable_v<F, Args...>, "Ensure function and arguments match!");

    auto data_copy = std::make_unique<T>(std::forward<F>(f), std::forward<Args>(args)...);

    constexpr auto invoker = _invoke_gen<T>(std::make_index_sequence<1+sizeof...(Args)>{});

    DWORD creation_flag = opt.priority.has_value() ? CREATE_SUSPENDED : 0;
    
    // Microsoft recommends _beginthreadex over CreateThread for C/C++ programs
    handle = reinterpret_cast<HANDLE>(_beginthreadex(
        nullptr,
        0,
        invoker,
        data_copy.get(),
        creation_flag,
        nullptr
    ));

    if ( !handle )
        throw std::system_error(errno, std::system_category());

    if ( creation_flag ) {
        // Redundant to check priority.has_value again until more are added
        int priority;

        switch ( opt.priority.value() /* Should never error */ ) {
            case Thread::Priority::LOWEST:
                priority = THREAD_PRIORITY_LOWEST;
                break;
            
            case Thread::Priority::LOW:
                priority = THREAD_PRIORITY_BELOW_NORMAL;
                break;
            
            case Thread::Priority::NORMAL:
                priority = THREAD_PRIORITY_NORMAL;
                break;
            
            case Thread::Priority::HIGH:
                priority = THREAD_PRIORITY_ABOVE_NORMAL;
                break;
            
            case Thread::Priority::HIGHEST:
                priority = THREAD_PRIORITY_HIGHEST;
                break;
            
            case Thread::Priority::TIME_CRITICAL:
                priority = THREAD_PRIORITY_TIME_CRITICAL;
                break;
            
            default: // In case I mess up - should never happen though...
                priority = THREAD_PRIORITY_NORMAL;
        }

        if ( !SetThreadPriority(handle, priority) ) {
            DWORD err = GetLastError();
            _cleanup_suspended(handle);
            throw std::system_error(err, std::system_category());
        }

        if ( ResumeThread(handle) == (DWORD)-1 ) {
            DWORD err = GetLastError();
            _cleanup_suspended(handle);
            throw std::system_error(err, std::system_category());
        }
    }

    data_copy.release(); // Will be cleaned up by invoker
}

inline bool _join(HANDLE& handle, size_t ms_timeout) {
    switch ( WaitForSingleObject(handle, ms_timeout) ) {
        case WAIT_OBJECT_0:
            CloseHandle(handle);
            handle = nullptr;
            return true;
        
        case WAIT_TIMEOUT:
            return false;

        default:
            throw std::system_error(GetLastError(), std::system_category());
    }
}

inline DWORD _thread_id(HANDLE handle) noexcept {
    return GetThreadId(handle);
}

inline void _detach(HANDLE& handle) {
    if ( !CloseHandle(handle) )
        throw std::system_error(GetLastError(), std::system_category());
    handle = nullptr;
}

inline void _force_join(HANDLE& handle) noexcept {
    WaitForSingleObject(handle, INFINITE);
    CloseHandle(handle);
    handle = nullptr;
}

inline unsigned int _hardware_concurrency() noexcept {
    SYSTEM_INFO sysinfo {0};
    GetSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
}

// =====================================================================
// Thread >> Implementations
// =====================================================================
Thread::Thread() noexcept: _handle(nullptr) {}

Thread::~Thread() {
    if (joinable()) {
        _force_join(_handle);
    }

}
Thread::Thread(Thread&& other) noexcept: Thread() { 
    std::swap(_handle, other._handle);
}

Thread& Thread::operator=(Thread&& other) { 
    if (joinable()) 
        join();
    std::swap(_handle, other._handle);
    return *this;
}

void Thread::swap(Thread& other) noexcept {
    std::swap(_handle, other._handle);
}

template <class F, class... Args>
Thread::Thread(F&& f, Args&&... args): Thread() {
    _start(_handle, {}, std::forward<F>(f), std::forward<Args>(args)...);
}

template <class F, class... Args>
Thread::Thread(Thread::Options opt, F&& f, Args&&... args): Thread() {
    _start(_handle, opt, std::forward<F>(f), std::forward<Args>(args)...);
}

bool Thread::joinable() const noexcept {
    return get_id() != this_thread::get_id();
}

Thread::id Thread::get_id() const noexcept {
    if ( _handle != nullptr )
        return id(_thread_id(_handle));
    return id();
}

void Thread::join() {
    if ( !joinable() )
        throw std::system_error(
            std::make_error_code(std::errc::invalid_argument),
            "Thread::join: thread not joinable"
        );
    _join(_handle, INFINITE);
}

bool Thread::join(size_t ms_timeout) {
    if ( !joinable() )
        throw std::system_error(
            std::make_error_code(std::errc::invalid_argument),
            "Thread::join: thread not joinable"
        );
    return _join(_handle, ms_timeout);
}

void Thread::detach() {
    if ( !joinable() )
        throw std::system_error(
            std::make_error_code(std::errc::invalid_argument),
            "Thread::detach: thread not detachable"
        );
    _detach(_handle);
}

Thread::native_handle_type Thread::native_handle() {
    if ( !joinable() )
        throw std::system_error(
            std::make_error_code(std::errc::invalid_argument),
            "Thread::native_handle: NULL-thread"
        );
    return _handle;
}

unsigned int Thread::hardware_concurrency() noexcept { 
    return _hardware_concurrency();
}
}

namespace std {
    inline void swap(simply::Thread& x, simply::Thread& y) noexcept {
        x.swap(y);
    }
}

#endif // SIMPLY_CONCURRENCY_HPP_