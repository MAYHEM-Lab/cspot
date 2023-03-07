add_subdirectory(tests)

add_library(
        dftype
        src/type_system/dftype.h
        src/type_system/dftype_builder.h
        src/type_system/dftype.c src/type_system/dftype_builder.c)


add_library(dfoperation src/dfoperation.c)
target_link_libraries(dfoperation PUBLIC woof)
target_include_directories(dfoperation PUBLIC "./include")

add_library(dfdebug src/dflogger.c src/dflogger.h src/type_system/dftype.c)

# TODO: fix wf->shared->filename
add_handler(dfhandler src/dfhandler.c)
target_link_libraries(dfhandler PUBLIC woof dfoperation dfdebug)
target_include_directories(dfhandler PUBLIC "./include")
