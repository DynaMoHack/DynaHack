# libnethack toplevel

#convenient names for inout directories
set (LNH_SRC  ${DynaMoHack_SOURCE_DIR}/libdynamohack/src)
set (LNH_DAT  ${DynaMoHack_SOURCE_DIR}/libdynamohack/dat)
set (LNH_UTIL ${DynaMoHack_SOURCE_DIR}/libdynamohack/util)
set (LNH_INC ${DynaMoHack_SOURCE_DIR}/libdynamohack/include)

# various generated files live here
set (LNH_SRC_GEN  ${DynaMoHack_BINARY_DIR}/libdynamohack/src)
set (LNH_DAT_GEN  ${DynaMoHack_BINARY_DIR}/libdynamohack/dat)
set (LNH_UTIL_GEN ${DynaMoHack_BINARY_DIR}/libdynamohack/util)
set (LNH_INC_GEN ${DynaMoHack_BINARY_DIR}/libdynamohack/include)

# actual target definitions etc.
add_subdirectory (util)
add_subdirectory (dat)
add_subdirectory (src)
