enable_testing()

add_library(tests_main tests_main.cpp)
target_compile_features(tests_main PUBLIC cxx_std_17)

add_executable(woofc_util_tests woofc_util_tests.cpp)
target_link_libraries(woofc_util_tests PRIVATE tests_main woof)
target_include_directories(woofc_util_tests PRIVATE ../include)
add_test(NAME woofc_util_tests COMMAND woofc_util_tests)