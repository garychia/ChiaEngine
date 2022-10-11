cmake_minimum_required(VERSION 3.22)
project(ChiaGameEngine)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

set(IMAGE_FILE_PATH ${CMAKE_SOURCE_DIR}/src/Display/Images/)
configure_file(src/Paths.hpp.in ${CMAKE_SOURCE_DIR}/src/Paths.hpp @ONLY)

include(CMakeDependentOption)

cmake_dependent_option(
    DIRECTX_ENABLED
    "Use DirectX as the rendering engine."
    ON "WIN32" OFF)
cmake_dependent_option(
    VULKAN_ENABLED
    "Use Vulkan as the rendering engine."
    ON "NOT DIRECTX_ENABLED" OFF
)
cmake_dependent_option(
    OPENGL_ENABLED
    "Use OpenGL as the rendering engine."
    ON "NOT DIRECTX_ENABLED;NOT VULKAN_ENABLED" OFF
)

if (NOT DIRECTX_ENABLED)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    include(FetchContent)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0.9.9.8
    )
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
    )
  FetchContent_MakeAvailable(glm glfw)
endif()

if(DIRECTX_ENABLED)
    add_compile_definitions(DIRECTX_ENABLED)
elseif(VULKAN_ENABLED)
    find_package(Vulkan REQUIRED)
    add_compile_definitions(VULKAN_ENABLED)
else()
    find_package(OpenGL REQUIRED)
    add_compile_definitions(OPENGL_ENABLED)
endif()

add_subdirectory(src)
add_subdirectory(test)
