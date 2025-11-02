// Tests for simply/concurrency library
// Uses Google Test framework
// 
// Note - Several timing tests use EXPECT instead of ASSERT.
//        These are fragile, and should be treated as indicators
//        rather than absolute validation

#include <simply/concurrency.h>
#include "gtest/gtest.h"

#include <unordered_set>
#include <string>
#include <sstream>
#include <system_error>
#include <atomic>

TEST(ThreadIdBasics, ThreadIdComparison) {
    simply::Thread::id id1;
    simply::Thread::id id2;

    ASSERT_EQ(id1, id2);
}


TEST(ThreadIdBasics, ThisThreadId) {
    simply::Thread::id id1;
    simply::Thread::id id2;

    ASSERT_EQ(id1, id2);
    ASSERT_EQ(id1, simply::this_thread::get_id());
}

TEST(ThreadIdBasics, ThreadIdHash) {
    std::unordered_set<simply::Thread::id> id_set;

    simply::Thread::id current = simply::this_thread::get_id();
    id_set.insert(current);

    ASSERT_EQ(id_set.size(), 1);
    ASSERT_EQ(id_set.count(current), 1);
}

TEST(ThreadIdBasics, ThreadIdUniqueness) {
    simply::Thread::id main = simply::this_thread::get_id();
    simply::Thread::id other;

    simply::Thread t([&other]() { other = simply::this_thread::get_id(); });

    simply::Thread::id comp = t.get_id();
    t.join();

    ASSERT_EQ(other, comp);
    ASSERT_NE(main, other);

    // After join, the threads id changes...
    ASSERT_NE(other, t.get_id());
    ASSERT_EQ(t.get_id(), main);
}

TEST(ThreadIdBasics, ThreadIdStreamable) {
    simply::Thread::id id = simply::this_thread::get_id();
    std::ostringstream oss;
    oss << id;

    ASSERT_FALSE(std::string(oss.str()).empty());
}

TEST(ThreadBasics, ThreadNull) {
    simply::Thread t;

    ASSERT_FALSE(t.joinable());
    ASSERT_EQ(t.get_id(), simply::this_thread::get_id());
    ASSERT_THROW(t.join(), std::system_error);
    ASSERT_THROW(t.detach(), std::system_error);
}

TEST(ThreadBasics, ThreadExecution) {
    bool executed_1 = false;
    bool executed_2 = false;

    simply::Thread t1([&executed_1]() {executed_1 = true;});
    t1.join();

    ASSERT_THROW(t1.join(), std::system_error);

    ASSERT_TRUE(executed_1);

    auto set_executed = [](bool& executed) { executed = true; };

    simply::Thread t3(set_executed, std::ref(executed_2));
    t3.join();

    ASSERT_TRUE(executed_2);
}

TEST(ThreadBasics, SetPriority) {
    simply::Thread t1({
        .priority = simply::Thread::Priority::HIGH
    }, [](){});
    ASSERT_EQ(t1.get_priority(), simply::Thread::Priority::HIGH);

    simply::Thread t2({
        .priority = simply::Thread::Priority::LOW
    }, []() {
        ASSERT_EQ(simply::this_thread::get_priority(), simply::Thread::Priority::LOW);
    });

    simply::Thread::Priority priorities[] = {
        simply::Thread::Priority::LOWEST,
        simply::Thread::Priority::LOW,
        simply::Thread::Priority::NORMAL,
        simply::Thread::Priority::HIGH,
        simply::Thread::Priority::HIGHEST
        // Not testing REAL_TIME as this may need special permission to run
    };

    for ( auto priority: priorities ) {
        bool executed = false;

        simply::Thread t({
            .priority = priority
        }, [&executed, priority](){
            executed = true;
            ASSERT_EQ(simply::this_thread::get_priority(), priority);
        });
        ASSERT_EQ(t.get_priority(), priority);
        t.join();
        ASSERT_TRUE(executed);
    }
}

TEST(ThreadBasics, ThreadDetach) {
    std::atomic<int> counter = 0;
    simply::Thread t1([&counter](){
        for ( int i = 0; i < 3; i++ ) {
            simply::this_thread::sleep(10);
            counter++;
        }
    });
    t1.detach();
    EXPECT_EQ(counter, 0);
    ASSERT_FALSE(t1.joinable());
    simply::this_thread::sleep(200);
    EXPECT_GT(counter, 0);
}

TEST(ThreadBasics, ThreadTimeout) {
    simply::Thread t1([](){ simply::this_thread::sleep(100); });
    EXPECT_FALSE(t1.join(0));

    simply::Thread t2([](){});
    EXPECT_TRUE(t2.join(100));
}

TEST(ThreadRAIIBasics, MoveConstructor) {
    bool executed = false;
    simply::Thread::id current;

    simply::Thread t1([&executed](){ executed = true; });

    ASSERT_TRUE(t1.joinable());
    ASSERT_NE(t1.get_id(), current);

    simply::Thread t2(std::move(t1));
    
    ASSERT_FALSE(t1.joinable());
    ASSERT_TRUE(t2.joinable());

    ASSERT_EQ(t1.get_id(), current);
    ASSERT_NE(t2.get_id(), current);

    ASSERT_THROW(t1.join(), std::system_error);
    ASSERT_NO_THROW(t2.join());
}

TEST(ThreadRAIIBasics, MoveAssignment) {
    bool first = false;
    bool second = false;

    simply::Thread t1([&first](){
        simply::this_thread::sleep(100);
        first = true;
    });

    simply::Thread t2([&second](){
        simply::this_thread::sleep(500);
        second = true;
    });

    ASSERT_FALSE(first || second);
    
    t1 = std::move(t2);

    ASSERT_TRUE(first);
    EXPECT_FALSE(second);
    
    ASSERT_FALSE(t2.joinable());
    ASSERT_TRUE(t1.joinable());
    
    ASSERT_THROW(t2.join(), std::system_error);
    ASSERT_NO_THROW(t1.join());
    ASSERT_TRUE(second);
}

TEST(ThreadRAIIBasics, ThreadDestructor) {
    bool executed = false;
    { simply::Thread t([&executed]() {
        simply::this_thread::sleep(100);
        executed = true;
      });
    };
    ASSERT_TRUE(executed);
}

TEST(ThreadRAIIBasics, ThreadSwap) {
    simply::Thread t1([](){});
    simply::Thread t2([](){});
    simply::Thread::id id1 = t1.get_id();
    simply::Thread::id id2 = t2.get_id();

    t1.swap(t2);

    ASSERT_EQ(t1.get_id(), id2);
    ASSERT_EQ(t2.get_id(), id1);
}

TEST(ThreadRAIIBasics, ThreadStdSwap) {
    simply::Thread t1([](){});
    simply::Thread t2([](){});
    simply::Thread::id id1 = t1.get_id();
    simply::Thread::id id2 = t2.get_id();

    std::swap(t1, t2);

    ASSERT_EQ(t1.get_id(), id2);
    ASSERT_EQ(t2.get_id(), id1);
}

TEST(ThreadSystemBasics, ThreadConcurrency) {
    EXPECT_GT(simply::Thread::hardware_concurrency(), 0);
}