#--------------------------------------------------------------------------
# exts
#--------------------------------------------------------------------------

ADD_LIBRARY(exts INTERFACE)

ADD_SUBDIRECTORY(flatbuffers)

ADD_SUBDIRECTORY(glm)
TARGET_LINK_LIBRARIES(exts INTERFACE glm_static)

ADD_SUBDIRECTORY(enet)
SET_TARGET_PROPERTIES(enet PROPERTIES FOLDER "exts/enet")
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE enet/include)
TARGET_LINK_LIBRARIES(exts INTERFACE enet)
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE flatbuffers/include)

if (WIN32)
    SET(SOLOUD_BACKEND_WINMM ON)
else ()
    SET(SOLOUD_BACKEND_ALSA ON)
endif ()
SET(SOLOUD_STATIC ON)

ADD_SUBDIRECTORY(soloud/contrib)
SET_TARGET_PROPERTIES(soloud PROPERTIES FOLDER "exts/soloud")
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE soloud/include)

SET(glew-cmake_BUILD_SHARED OFF)
SET(ONLY_LIBS ON)
ADD_SUBDIRECTORY(glew)
#SET_TARGET_PROPERTIES(glew PROPERTIES FOLDER "exts/glew")
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE glew/include)

TARGET_LINK_LIBRARIES(exts INTERFACE libglew_static)

SET(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
SET(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
SET(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
SET(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")

ADD_SUBDIRECTORY(glfw)
TARGET_INCLUDE_DIRECTORIES(exts INTERFACE glfw/include)
TARGET_LINK_LIBRARIES(exts INTERFACE glfw)

FILE(GLOB imgui_sources
        imgui/*.cpp
        imgui/backends/imgui_impl_glfw.cpp
        imgui/backends/imgui_impl_opengl3.cpp
)
ADD_LIBRARY(imgui STATIC ${imgui_sources})
TARGET_LINK_LIBRARIES(imgui PUBLIC glfw)
TARGET_INCLUDE_DIRECTORIES(imgui PUBLIC imgui PUBLIC imgui/backends)

TARGET_LINK_LIBRARIES(exts INTERFACE imgui)
