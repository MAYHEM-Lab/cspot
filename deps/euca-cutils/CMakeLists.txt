add_library(euca_utils 
	aggr.c
	convert_time.c
	dlist.c
	ptime.c
	redblack.c
	simple_input.c
	textlist.c)

target_include_directories(euca_utils PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<INSTALL_INTERFACE:include>
)

file(GLOB EUCA_HEADERS "*.h")
install(FILES ${EUCA_HEADERS} DESTINATION include)