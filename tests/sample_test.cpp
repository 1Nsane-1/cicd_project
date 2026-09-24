#include "../src/test_framework.h"

TEST(Math, Addition) {
    ASSERT_EQ(2 + 2, 4);
}

TEST(String, Equality) {
    std::string s = "cicd";
    ASSERT_TRUE(s == "cicd");
}

int main() {
    auto results = TestRegistry::instance().run_parallel();
    bool all_passed = true;
    for (auto& r : results) {
        if (!r.passed) {
            std::cout << "[FAIL] " << r.name << ": " << r.error_message << std::endl;
            all_passed = false;
        } else {
            std::cout << "[PASS] " << r.name << std::endl;
        }
    }
    return all_passed ? 0 : 1;
}