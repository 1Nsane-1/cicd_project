#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <future>
#include <mutex>

struct TestResult {
    std::string name;
    bool passed;
    std::string error_message;
};

class TestRegistry {
    struct TestItem {
        std::string name;
        std::function<void()> fn;
    };
    std::vector<TestItem> tests;

public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }

    void add(const std::string& name, std::function<void()> fn) {
        tests.push_back({name, fn});
    }

    std::vector<TestResult> run_parallel() {
        std::vector<std::future<TestResult>> futures;
        for (auto& t : tests) {
            futures.push_back(std::async(std::launch::async, [t]() {
                TestResult res{t.name, true, ""};
                try {
                    t.fn();
                } catch (const std::exception& e) {
                    res.passed = false;
                    res.error_message = e.what();
                } catch (...) {
                    res.passed = false;
                    res.error_message = "Unknown error";
                }
                return res;
            }));
        }

        std::vector<TestResult> results;
        for (auto& f : futures) {
            results.push_back(f.get());
        }
        return results;
    }
};

#define TEST(group, name) \
    void test_##group##_##name(); \
    struct Register_##group##_##name { \
        Register_##group##_##name() { \
            TestRegistry::instance().add(#group "." #name, test_##group##_##name); \
        } \
    } reg_##group##_##name; \
    void test_##group##_##name()

#define ASSERT_TRUE(cond) \
    if (!(cond)) throw std::runtime_error("Assertion failed: " #cond " at line " + std::to_string(__LINE__))

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) throw std::runtime_error("Assertion failed: " #a " == " #b " at line " + std::to_string(__LINE__))

#endif