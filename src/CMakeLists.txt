set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src)
if(NOT DIRECTX_ENABLED)
    list(APPEND INCLUDE_DIR ${glfw_SOURCE_DIR}/include)
    list(APPEND INCLUDE_DIR ${glm_SOURCE_DIR})
endif()
if(VULKAN_ENABLED)
    list(APPEND INCLUDE_DIR ${Vulkan_INCLUDE_DIRS})
endif()

add_subdirectory(App)
add_subdirectory(Data)
add_subdirectory(Display)
add_subdirectory(Geometry)
add_subdirectory(Math)
add_subdirectory(System)


set(COMMON_SOURCE_FILES Globals.hpp pch.hpp Paths.hpp)

if(WIN32)
    add_executable(ChiaEngine WIN32
        WinMain.cpp
        ${COMMON_SOURCE_FILES})
else()
    add_executable(ChiaEngine
        main.cpp
        ${COMMON_SOURCE_FILES})
endif()

set(CHIA_LIBRARIES
    ChiaApp
    ChiaSystem
    ChiaDisplay
    ChiaGeometry
)

if (DIRECTX_ENABLED)
    list(APPEND CHIA_LIBRARIES ChiaDX)
elseif(VULKAN_ENABLED)
    list(APPEND CHIA_LIBRARIES ChiaVulkan)
else()
    list(APPEND CHIA_LIBRARIES ChiaOpenGL)
endif()

target_link_libraries(ChiaEngine ${CHIA_LIBRARIES})
target_include_directories(ChiaEngine PRIVATE
    ${INCLUDE_DIR}
)
target_precompile_headers(ChiaEngine PRIVATE pch.hpp)
