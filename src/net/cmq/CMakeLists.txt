#add_library(woof_cmq_net zmq-backend.cpp zmq_wrap.hpp server.cpp common.hpp common.cpp backend.hpp endpoint.cpp backend.cpp client.cpp)
add_library(woof_cmq_net cmq_wrap.hpp backend.hpp backend.cpp common.hpp common.cpp client.cpp server.cpp cmq-backend.cpp cmq-pkt.c cmq-frame.c)
target_link_libraries(woof_cmq_net PUBLIC woof)
target_include_directories(woof_cmq_net PRIVATE "." "../../include")
