cmake_minimum_required(VERSION 3.10)
project(MapWoader)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(
    external
    external/imgui
    external/imgui/backends
    external/glad/include
    external/glfw/include
    external/glm
    external/tinyxml2
    external/stb
    external/engine
    external/engine/Assets
    external/engine/Math
    external/engine/Objects
    external/engine/Reflection
    src
)

# Source files
file(GLOB_RECURSE SRC_FILES
    src/*.cpp
    external/imgui/*.cpp
    external/imgui/backends/imgui_impl_glfw.cpp
    external/imgui/backends/imgui_impl_opengl3.cpp
    external/glad/src/glad.c
    external/tinyxml2/tinyxml2.cpp
    external/engine/*.cpp
    external/engine/*/*.cpp
)

add_subdirectory(external/glfw)

add_executable(MapWoader ${SRC_FILES})

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

target_link_libraries(MapWoader glfw ${CMAKE_DL_LIBS})

if (WIN32)
    target_link_libraries(MapWoader opengl32)
endif()

add_custom_command(TARGET MapWoader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:MapWoader>/shaders
)

add_custom_command(TARGET MapWoader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:MapWoader>/resources
)
