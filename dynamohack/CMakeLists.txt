# build the curses window port of the game


if (NOT WIN32)
    set (USE_PDCURSES FALSE CACHE BOOL "Use PDCurses?")
    set (PDCURSES_LIBNAME "XCurses")
else ()
    set (USE_PDCURSES TRUE)
    set (PDCURSES_LIBNAME "pdcurses")
endif ()


if (USE_PDCURSES)
    set (PDCURSES_LIB_DIR "" CACHE PATH "Location of the pdcurses library")
    set (PDCURSES_INC_DIR "" CACHE PATH "Location of the pdcurses header")
    link_directories (${PDCURSES_LIB_DIR})
    include_directories (${PDCURSES_INC_DIR})
    add_definitions (-DPDCURSES)
endif ()
if (USE_OSX_HOMEBREW_CURSES)
    add_definitions(-DUSE_OSX_HOMEBREW_CURSES)
endif()


set (NH_CURSES_SRC
    src/main.c
    src/boxdraw.c
    src/color.c
    src/dialog.c
    src/gameover.c
    src/getline.c
    src/keymap.c
    src/map.c
    src/menu.c
    src/messages.c
    src/options.c
    src/outchars.c
    src/playerselect.c
    src/replay.c
    src/rungame.c
    src/sidebar.c
    src/status.c
    src/topten.c
    src/windows.c)
if (ENABLE_NETCLIENT)
    set (NH_CURSES_SRC
        ${NH_CURSES_SRC}
        src/netgame.c
        src/netplay.c)
endif ()
if (WIN32)
    set (NH_CURSES_SRC ${NH_CURSES_SRC} rc/nh_ico.rc)
endif ()

include_directories (${DynaMoHack_SOURCE_DIR}/include
                     include)

add_definitions(-DDYNAMOHACKDIR="${DATADIR}")
if (ENABLE_NETCLIENT)
    add_definitions(-DNETCLIENT)
endif ()

link_directories (${DynaMoHack_BINARY_DIR}/libdynamohack/src)
link_directories (${DynaMoHack_BINARY_DIR}/libdynamohack_client)
add_executable (dynamohack_curses ${NH_CURSES_SRC} )
set_target_properties(dynamohack_curses PROPERTIES OUTPUT_NAME dynamohack)
target_link_libraries(dynamohack_curses dynamohack)
if (ALL_STATIC)
    # kludge like the one below for jansson and libdynamohack_client.
    # It's actually libdynamohack that requires zlib, not dynamohack_curses,
    # but CMake can't chain dependencies transitively, AFAIK.
    target_link_libraries(dynamohack_curses z)
else ()
    if (UNIX)
        set_target_properties (dynamohack_curses PROPERTIES INSTALL_RPATH "${GAME_RPATH}")
    endif ()
endif ()

if (USE_PDCURSES)
    target_link_libraries (dynamohack_curses ${PDCURSES_LIBNAME})
else ()
    if(USE_OSX_HOMEBREW_CURSES)
        target_link_libraries (dynamohack_curses ncurses)
    else ()
        target_link_libraries (dynamohack_curses ncursesw)
    endif ()
endif ()

if (ENABLE_NETCLIENT)
    add_dependencies (dynamohack_curses libdynamohack_client)
    target_link_libraries (dynamohack_curses dynamohack_client z)
    # this is a kludge; these libraries are required by libdynamohack_client, but
    # for static linking they must be specified here. I couldn't figure out how
    # to set the dependencies from the libdynamohack_client CMakeLists.txt
    if (ALL_STATIC)
	target_link_libraries(dynamohack_curses jansson)
	if (WIN32)
	    target_link_libraries(dynamohack_curses Ws2_32)
	endif ()
    endif()
endif ()


add_dependencies (dynamohack_curses libdynamohack)

if (UNIX)
    add_custom_command(OUTPUT dynamohack.sh
                       COMMAND printf "%s\\n%s\\n" "#!/bin/sh"
                               "exec ${BINDIR}/dynamohack $@"
                               >> dynamohack.sh
                       VERBATIM)
    add_custom_target(dynamohack_curses_shell_script ALL
                      DEPENDS dynamohack.sh)
    install(PROGRAMS ${DynaMoHack_BINARY_DIR}/dynamohack/dynamohack.sh
            DESTINATION ${SHELLDIR})
endif()

install(TARGETS dynamohack_curses
        DESTINATION ${BINDIR})
