if (NOT Boost_FOUND)
  return()
endif()

if (NOT LAPACK_FOUND)
  return()
endif()

find_package(Boost REQUIRED)
find_package(LAPACK REQUIRED)

include_directories("./include" ${Boost_INCLUDE_DIR} ${MLPACK_INCLUDE_DIR})


add_handler(h_predict h_predict.cpp)
target_link_libraries(h_predict PUBLIC dht ${Boost_LIBRARIES})

add_handler(h_smooth h_smooth.cpp)
target_link_libraries(h_smooth PUBLIC dht)

add_handler(h_train h_train.cpp)
target_link_libraries(h_train PUBLIC dht ${Boost_LIBRARIES})

add_executable(init_topic init_topic.c)
target_link_libraries(init_topic PUBLIC woof dht)

add_executable(list_error list_error.c)
target_link_libraries(list_error PUBLIC woof dht)

add_executable(publish_temp publish_temp.c)
target_link_libraries(publish_temp PUBLIC woof dht)