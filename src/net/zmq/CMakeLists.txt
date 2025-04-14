add_library(woof_zmq_net zmq-backend.cpp zmq_wrap.hpp server.cpp common.hpp common.cpp backend.hpp endpoint.cpp backend.cpp client.cpp)
target_link_libraries(woof_zmq_net PUBLIC woof woof_caplets crypto dl yaml pthread)
target_link_libraries(woof_zmq_net PUBLIC czmq-static)
target_include_directories(woof_zmq_net PRIVATE "../../include" "../../caplets")
