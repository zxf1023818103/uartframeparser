# Lua can be compiled as either C or C++.
# Default configuration is C, set COMPILE_AS_CPP to ON to use C++.
# See http://stackoverflow.com/questions/13560945/c-and-c-library-using-longjmp for why would you want to do that.
# Primary differences:
# - Exceptions will be used instead of setjmp/longjmp
# - The name mangling for functions will be C++ instead of C.
#     - This is a source-incompatible change because extern "C" is chosen by the including application.
# - The lua.hpp header will not be available.

PROJECT ( lua )


CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET (HDR_LIBLUA
lapi.h lauxlib.h lcode.h lctype.h ldebug.h ldo.h lfunc.h
lgc.h llex.h llimits.h lmem.h lobject.h lopcodes.h lparser.h
lstate.h lstring.h ltable.h ltm.h lua.h luaconf.h lualib.h
lundump.h lvm.h lzio.h
)

# Build Libraries
SET (SRC_LIBLUA
lapi.c lauxlib.c lbaselib.c lcode.c lcorolib.c
lctype.c ldblib.c ldebug.c ldo.c ldump.c lfunc.c lgc.c
linit.c liolib.c llex.c lmathlib.c lmem.c loadlib.c lobject.c
lopcodes.c loslib.c lparser.c lstate.c lstring.c lstrlib.c
ltable.c ltablib.c ltm.c lundump.c lutf8lib.c lvm.c lzio.c
)

#IF (COMPILE_AS_CPP)
#    SET_SOURCE_FILES_PROPERTIES(${SRC_LIBLUA} lua.c luac.c PROPERTIES LANGUAGE CXX)
#ELSE ()
    SET (CMAKE_C_STANDARD 99)
#ENDIF ()

# append headers to sources to make them show up in MSVC GUI
LIST(APPEND SRC_LIBLUA ${HDR_LIBLUA})

IF (WIN32)
    # remove warnings
    ADD_DEFINITIONS (-D_CRT_SECURE_NO_WARNINGS )
ENDIF ()

IF (UNIX)
    ADD_DEFINITIONS (-DLUA_USE_POSIX)
    FIND_LIBRARY (LIB_MATH NAMES m)
    IF (LIB_MATH)
        LINK_LIBRARIES (${LIB_MATH})
    ENDIF ()
ENDIF ()

#DLL
ADD_LIBRARY ( lua STATIC ${SRC_LIBLUA} )
SET_PROPERTY (TARGET lua PROPERTY POSITION_INDEPENDENT_CODE ON)

IF (BUILD_SHARED_LIBS AND WIN32)
    TARGET_COMPILE_DEFINITIONS (lua PUBLIC -DLUA_BUILD_AS_DLL )
ENDIF ()

IF (UNIX)
    IF (APPLE)
        TARGET_COMPILE_DEFINITIONS (lua PUBLIC -DLUA_USE_DLOPEN)
    ELSE ()
        FIND_LIBRARY (LIB_DLOPEN NAMES dl)
        IF (LIB_DLOPEN)
            TARGET_COMPILE_DEFINITIONS (lua PUBLIC -DLUA_USE_DLOPEN)
            TARGET_LINK_LIBRARIES (lua ${LIB_DLOPEN})
        ENDIF ()
    ENDIF ()
ENDIF ()

# INSTALL ( TARGETS lua
#     EXPORT unofficial-lua-config
#     RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
#     LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
#     ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
# )

# install(EXPORT unofficial-lua-config DESTINATION share/unofficial-lua)

# IF (COMPILE_AS_CPP)
#     ADD_LIBRARY ( lua-cpp ${SRC_LIBLUA} )
#     SET_TARGET_PROPERTIES(lua-cpp PROPERTIES OUTPUT_NAME "lua-c++")
#     IF (BUILD_SHARED_LIBS AND WIN32)
#         TARGET_COMPILE_DEFINITIONS (lua-cpp PUBLIC -DLUA_BUILD_AS_DLL )
#     ENDIF ()
#     IF (UNIX)
#         IF (APPLE)
#             TARGET_COMPILE_DEFINITIONS (lua-cpp PUBLIC -DLUA_USE_DLOPEN)
#         ELSE ()
#             FIND_LIBRARY (LIB_DLOPEN NAMES dl)
#             IF (LIB_DLOPEN)
#                 TARGET_COMPILE_DEFINITIONS (lua-cpp PUBLIC -DLUA_USE_DLOPEN)
#                 TARGET_LINK_LIBRARIES (lua-cpp ${LIB_DLOPEN})
#             ENDIF ()
#         ENDIF ()
#     ENDIF ()

#     INSTALL ( TARGETS lua-cpp
#         EXPORT unofficial-lua-cpp-config
#         RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
#         LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
#         ARCHIVE DESTINATION ${CMAKE_INSTALL_PREFIX}/lib
#     )
    
#     install(EXPORT unofficial-lua-cpp-config DESTINATION share/unofficial-lua-cpp)
# ENDIF()

# IF (INSTALL_TOOLS)
#     ADD_EXECUTABLE ( luac luac.c ${SRC_LIBLUA} ) # compiler uses non-exported APIs, so must include sources directly.
#     ADD_EXECUTABLE ( luai lua.c )  # interpreter
#     TARGET_LINK_LIBRARIES ( luai lua )
#     SET_TARGET_PROPERTIES ( luai PROPERTIES OUTPUT_NAME lua PDB_NAME luai )
#     IF (UNIX)
#         IF (CMAKE_SYSTEM_NAME STREQUAL FreeBSD)
#             SET (_LIB_READLINE_NAME edit)
#         ELSE ()
#             SET (_LIB_READLINE_NAME readline)
#         ENDIF ()
#         FIND_LIBRARY (LIB_READLINE NAMES ${_LIB_READLINE_NAME})
#         IF (LIB_READLINE)
#             TARGET_COMPILE_DEFINITIONS (luai PUBLIC -DLUA_USE_READLINE)
#             TARGET_LINK_LIBRARIES(luai ${LIB_READLINE})
#             IF (_LIB_READLINE_NAME STREQUAL edit)
#                 TARGET_INCLUDE_DIRECTORIES (luai PUBLIC /usr/include/edit)
#             ENDIF ()
#         ENDIF ()
#     ENDIF ()
#     INSTALL ( TARGETS luai luac RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}/tools/lua )
# ENDIF ()

# IF (NOT SKIP_INSTALL_HEADERS)
#     INSTALL(
#         FILES
#             lualib.h
#             lua.h
#             luaconf.h
#             lauxlib.h
#         DESTINATION include
#     )
#     # If using C++, don't install extern "C" wrapper.
#     IF (NOT COMPILE_AS_CPP)
#         INSTALL(FILES lua.hpp DESTINATION include)
#     ENDIF ()
# ENDIF ()
