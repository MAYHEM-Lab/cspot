enable_testing()

add_library(tests_main tests_main.cpp)

add_executable(woofc_util_tests woofc_util_tests.cpp)
target_link_libraries(woofc_util_tests PRIVATE tests_main woof)
add_test(NAME woofc_util_tests COMMAND woofc_util_tests)