# network client library

if (WIN32 OR CYGWIN)
    if (CYGWIN)
	set (JANSSON_INC_DIR "/usr/local/include" CACHE PATH "Location of the jansson JSON headers")
	set (JANSSON_LIB_DIR "/usr/local/lib" CACHE PATH "Location of the jansson JSON library")
    else ()
	set (JANSSON_INC_DIR "" CACHE PATH "Location of the jansson JSON headers")
	set (JANSSON_LIB_DIR "" CACHE PATH "Location of the jansson JSON library")
    endif ()
    include_directories(${JANSSON_INC_DIR})
    link_directories(${JANSSON_LIB_DIR})
endif ()


set (LIBDYNAMOHACK_CLIENT_SRC
    src/clientapi.c
    src/connection.c
    src/netcmd.c
    ${DynaMoHack_SOURCE_DIR}/libdynamohack/src/xmalloc.c
    )

include_directories (${DynaMoHack_SOURCE_DIR}/include
                     include)

link_directories (${DynaMoHack_BINARY_DIR}/libdynamohack/src)
add_library(libdynamohack_client ${LIB_TYPE} ${LIBDYNAMOHACK_CLIENT_SRC})
set_target_properties(libdynamohack_client PROPERTIES OUTPUT_NAME dynamohack_client)

if (NOT ALL_STATIC)
    target_link_libraries(libdynamohack_client dynamohack jansson)
    if (WIN32)
	target_link_libraries(libdynamohack_client Ws2_32)
    endif ()
endif ()

add_dependencies (libdynamohack_client libdynamohack)

install(TARGETS libdynamohack_client
        DESTINATION ${BINDIR})
