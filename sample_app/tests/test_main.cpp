#include "test_framework.hpp"

int add(int a, int b) { return a + b; }

TEST(MathTests, AdditionTest) {
    ASSERT_EQ(add(2, 3), 5);
}

TEST(MathTests, FailCheck) {
    ASSERT_TRUE(add(10, 20) == 30);
}

TEST(StringTests, LengthTest) {
    std::string text = "ALTLinux";
    ASSERT_EQ(text.length(), 8);
}

int main() {
    return RunAllTestsParallel();
}
