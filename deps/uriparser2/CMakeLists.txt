add_library(uriparser2 uriparser2.c
        uriparser2.h
        uriparser/UriParse.c
        uriparser/UriParseBase.c
        uriparser/UriCommon.c
        uriparser/UriIp4.c
        uriparser/UriIp4Base.c)
target_include_directories(uriparser2 PUBLIC .)
