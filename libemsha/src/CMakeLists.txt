project(libemsha)

add_library(emsha 
    emsha/sha256.hh
    emsha/hmac.hh
    emsha/emsha.hh
    emsha.cc
    hmac.cc
    sha256.cc
    internal.hh
)
target_include_directories(emsha PUBLIC .)
target_compile_features(emsha PUBLIC cxx_std_14)
if (TOS)
    target_compile_definitions(emsha PUBLIC TOS)
    target_link_libraries(emsha PUBLIC tos_util)
endif()

if (NOT TOS)
add_library(test_utils test_utils.cc test_utils.hh)
target_link_libraries(test_utils PUBLIC emsha)

add_executable(test_emsha test_emsha.cc)
target_link_libraries(test_emsha test_utils)

add_executable(test_hmac test_hmac.cc)
target_link_libraries(test_hmac test_utils)

add_executable(test_mem test_mem.cc)
target_link_libraries(test_mem test_utils)

add_executable(test_sha256 test_sha256.cc)
target_link_libraries(test_sha256 test_utils)

add_test(
    "Test Emsha"
    test_emsha
)
endif()