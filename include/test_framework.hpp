#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <future>
#include <mutex>
#include <sstream>

struct TestCase {
    std::string suite;
    std::string name;
    std::function<void(std::string&, bool&)> func;
};

class TestRegistry {
public:
    static TestRegistry& instance() {
        static TestRegistry reg;
        return reg;
    }
    void addTest(const std::string& suite, const std::string& name, std::function<void(std::string&, bool&)> fn) {
        tests.push_back({suite, name, fn});
    }
    const std::vector<TestCase>& getTests() const { return tests; }
private:
    std::vector<TestCase> tests;
};

struct TestRegistrar {
    TestRegistrar(const std::string& suite, const std::string& name, std::function<void(std::string&, bool&)> fn) {
        TestRegistry::instance().addTest(suite, name, fn);
    }
};

#define TEST(suite_name, test_name) \
    void suite_name##_##test_name##_fn(std::string& _err_msg, bool& _passed); \
    static TestRegistrar suite_name##_##test_name##_reg(#suite_name, #test_name, suite_name##_##test_name##_fn); \
    void suite_name##_##test_name##_fn(std::string& _err_msg, bool& _passed)

#define ASSERT_EQ(val1, val2) do { \
    auto _v1 = (val1); \
    auto _v2 = (val2); \
    if (_v1 != _v2) { \
        std::ostringstream _ss; \
        _ss << "Сбой в " << __FILE__ << ":" << __LINE__ << " (" << #val1 << " [" << _v1 << "] != " << #val2 << " [" << _v2 << "])"; \
        _err_msg = _ss.str(); \
        _passed = false; \
        return; \
    } \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        std::ostringstream _ss; \
        _ss << "Сбой в " << __FILE__ << ":" << __LINE__ << " (" << #cond << " ложно)"; \
        _err_msg = _ss.str(); \
        _passed = false; \
        return; \
    } \
} while(0)

struct TestResult {
    std::string suite;
    std::string name;
    bool passed;
    std::string error;
};

inline int RunAllTestsParallel() {
    const auto& tests = TestRegistry::instance().getTests();
    std::vector<TestResult> results;
    std::mutex result_mutex;

    std::vector<std::future<void>> futures;
    for (const auto& test : tests) {
        futures.push_back(std::async(std::launch::async, [&test, &results, &result_mutex]() {
            std::string err;
            bool passed = true;
            try {
                test.func(err, passed);
            } catch (const std::exception& e) {
                passed = false;
                err = std::string("Исключение: ") + e.what();
            } catch (...) {
                passed = false;
                err = "Неизвестная ошибка";
            }
            std::lock_guard<std::mutex> lock(result_mutex);
            results.push_back({test.suite, test.name, passed, err});
        }));
    }

    for (auto& f : futures) {
        f.get();
    }

    int passed_cnt = 0, failed_cnt = 0;
    std::cout << "\n=== РЕЗУЛЬТАТЫ МОДУЛЬНОГО ТЕСТИРОВАНИЯ (std::async) ===\n";
    for (const auto& res : results) {
        if (res.passed) {
            std::cout << " [ УСПЕХ ] " << res.suite << "." << res.name << "\n";
            passed_cnt++;
        } else {
            std::cout << " [ ОШИБКА ] " << res.suite << "." << res.name << " -> " << res.error << "\n";
            failed_cnt++;
        }
    }
    std::cout << "-----------------------------------------------------\n";
    std::cout << "Пройдено: " << passed_cnt << " | Провалено: " << failed_cnt << "\n\n";
    return (failed_cnt == 0) ? 0 : 1;
}
