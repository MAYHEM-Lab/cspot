find_package(FlatBuffers REQUIRED)

FLATBUFFERS_GENERATE_C_HEADERS(flats flats/woof.fbs flats/requests.fbs flats/capability_request.fbs)
add_library(cspot_net ${flats_OUTPUTS} stub.cpp)
target_include_directories(cspot_net PUBLIC ${CMAKE_BINARY_DIR})
