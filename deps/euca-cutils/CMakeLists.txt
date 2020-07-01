add_library(euca_utils 
	aggr.c
	convert_time.c
	dlist.c
	ptime.c
	redblack.c
	simple_input.c
	textlist.c)

target_include_directories(euca_utils PUBLIC .)
