#pragma once
#include <string>
#include <vector>
#include <functional>
#include <cstdio>
#include <cmath>
#include <stdexcept>

struct TestCase {
    const char* name;
    std::function<void()> fn;
};

class TestRunner {
public:
    static TestRunner& get() {
        static TestRunner runner;
        return runner;
    }
    
    void add(const char* n, std::function<void()> f) {
        tests.push_back({n, f});
    }
    
    int runAll() {
        int pass = 0, fail = 0;
        for (auto& t : tests) {
            try {
                t.fn();
                pass++;
                printf("[PASS] %s\n", t.name);
            } catch (const std::exception& e) {
                fail++;
                printf("[FAIL] %s: %s\n", t.name, e.what());
            } catch (...) {
                fail++;
                printf("[FAIL] %s: unknown error\n", t.name);
            }
        }
        printf("\n=== %d passed, %d failed ===\n", pass, fail);
        return fail;
    }
    
private:
    std::vector<TestCase> tests;
};

#define TEST(name) namespace test_##name { \
    void run(); \
    static bool reg = ([](){ \
        TestRunner::get().add(#name, run); return true; }()); \
    } \
    void run()

#define ASSERT_EQ(a, b) do { if ((a) != (b)) throw std::runtime_error(#a " != " #b); } while(0)
#define ASSERT_TRUE(x) do { if (!(x)) throw std::runtime_error(#x); } while(0)
#define ASSERT_FALSE(x) do { if (x) throw std::runtime_error(#x " is true"); } while(0)
#define ASSERT_FLOAT_EQ(a, b, e) do { if (std::abs((a)-(b)) > (e)) throw std::runtime_error(#a " != " #b); } while(0)