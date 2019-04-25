
add_library(caps include/caps/common.hpp include/caps/caps.hpp include/caps/emsha_signer.hpp include/caps/raw_serdes.hpp include/caps/json_serdes.hpp include/caps/request.hpp)
target_include_directories(caps PUBLIC include)
target_link_libraries(caps PUBLIC tos_util emsha cwpack)