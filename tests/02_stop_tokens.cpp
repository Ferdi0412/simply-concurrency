// Tests for simply/concurrency library
// Uses Google Test framework
//
// Note - This requires C++ >= 20

#include <simply/concurrency.h>
#include "gtest/gtest.h"

#if SIMPLY_C20plus

#include <stop_token>

TEST(ThreadStopBasics, TokenWorks) {
    bool first_seen = false;
    bool second_seen = false;
    std::stop_source stop;

    auto callback = [](std::stop_token stop, bool& report) {
        while ( !stop.stop_requested() )
            simply::this_thread::sleep(10);
        report = true;
    };

    simply::Thread t1(callback, stop.get_token(), std::ref(first_seen));
    stop.request_stop();
    simply::this_thread::sleep(100);
    EXPECT_TRUE(first_seen);

    simply::Thread t2(callback, std::ref(second_seen));
    ASSERT_FALSE(second_seen);
    t2.join();
    ASSERT_TRUE(second_seen);
} 

TEST(ThreadStopBasics, GetStopSource) {
    bool executed = false;
    simply::Thread t1([&executed](std::stop_token stop){
        while( !stop.stop_requested() )
            simply::this_thread::sleep(10);
        executed = true;
    });

    t1.get_stop_source().request_stop();
    simply::this_thread::sleep(100);
    EXPECT_TRUE(executed);
}

TEST(ThreadStopBasics, GetStopToken) {
    simply::Thread t1([](std::stop_token stop){});

    ASSERT_FALSE(t1.get_stop_token().stop_requested());
    t1.join();
    ASSERT_TRUE(t1.get_stop_token().stop_requested());
}

#endif