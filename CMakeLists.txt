#--------------------------------------------------------------------------
# spaceengine
#--------------------------------------------------------------------------
PROJECT(spaceengine)
CMAKE_MINIMUM_REQUIRED(VERSION 3.2)

if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(BUILD_FOR_WIN TRUE)
endif ()

option(STATIC_BUILD "Build a static binary" ${BUILD_FOR_WIN})

if (STATIC_BUILD)
    set(CMAKE_EXE_LINKER_FLAGS "-static")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" CONFIG)
    set(BUILD_SHARED_LIBS OFF)
endif ()

MACRO(TARGET_PCH target path)
    IF (WIN32)
        IF (MSVC)
            SET_TARGET_PROPERTIES(${target}
                    PROPERTIES
                    COMPILE_FLAGS "/Yuconfig.h /Fp${CMAKE_CFG_INTDIR}/${target}.pch")
            SET_SOURCE_FILES_PROPERTIES(${path}config.cc
                    PROPERTIES
                    COMPILE_FLAGS "/Ycconfig.h")
        ENDIF ()
    ENDIF ()
ENDMACRO(TARGET_PCH)

if (MSVC)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20")
endif (MSVC)
IF (CMAKE_COMPILER_IS_GNUCXX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")
ENDIF ()
IF (APPLE)
    SET(CMAKE_CXX_FLAGS "-g -std=c++20 -stdlib=libc++")
ENDIF ()

IF (MSVC)
    SET(OPENGL_LIBS opengl32.lib)
ELSE ()
    SET(OPENGL_LIBS GL GLU X11 Xxf86vm pthread Xrandr Xi Xinerama Xcursor)
ENDIF ()

SET(GSCEPT_LAB_ENV_ROOT ${CMAKE_CURRENT_DIR})

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

SET_PROPERTY(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS GLEW_STATIC)

ADD_SUBDIRECTORY(exts)

ADD_LIBRARY(pch INTERFACE pch/config.h pch/config.cc)
TARGET_INCLUDE_DIRECTORIES(pch INTERFACE pch/)
TARGET_LINK_LIBRARIES(pch INTERFACE exts)

ADD_SUBDIRECTORY(engine)
TARGET_PRECOMPILE_HEADERS(engine INTERFACE pch/config.h)
ADD_SUBDIRECTORY(projects)

