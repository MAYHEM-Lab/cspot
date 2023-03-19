#include <iostream>
#include <string>
#include <cstdio>
#include <unistd.h>

#define TEST(name)                       \
    bool assertion_failed = false;       \
    system("sudo rm -f laminar-*"); \
    reset(); /* reset setup data structures */ \
    std::cout << name << ": ";

#define END_TEST()                          \
    if (!assertion_failed) {                \
        std::cout << "PASSED" << std::endl; \
    }

#define ASSERT(condition, comment)                                          \
    if (!(condition)) {                                                     \
        if (!assertion_failed) {                                            \
            assertion_failed = true;                                        \
            std::cout << "FAILED" << std::endl;                             \
        }                                                                   \
        std::cout << "\t" << __FUNCTION__ << " failed on line " << __LINE__ \
                  << std::endl;                                             \
        std::cout << "\t\"" << comment << "\"" << std::endl;                \
    }

#define ASSERT_EQ(actual, expected, comment)                                \
    if ((actual) != (expected)) {                                           \
        if (!assertion_failed) {                                            \
            assertion_failed = true;                                        \
            std::cout << "FAILED" << std::endl;                             \
        }                                                                   \
        std::cout << "\t" << __FUNCTION__ << " failed on line " << __LINE__ \
                  << std::endl;                                             \
        std::cout << "\t\"" << comment << "\"" << std::endl;                \
        std::cout << "\t EXPECTED: " << expected << std::endl;              \
        std::cout << "\t ACTUAL: " << actual << std::endl;                  \
    }
