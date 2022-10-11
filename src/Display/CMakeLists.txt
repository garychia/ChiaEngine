if (DIRECTX_ENABLED)
    add_subdirectory(DirectX)
elseif(VULKAN_ENABLED)
    add_subdirectory(Vulkan)
else()
    add_subdirectory(OpenGL)
endif()
set(DISPLAY_SOURCE_FILES
    Camera.hpp
    Camera.cpp
    Color.hpp
    Color.cpp
    IRenderable.hpp
    IRenderable.cpp
    IRenderer.hpp
    Renderer.hpp
    Renderer.cpp
    Scene.hpp
    Scene.cpp
    Shader.hpp
    Shader.cpp
    Texture.hpp
    Texture.cpp
    Window.hpp
    WindowInfo.hpp
    WindowInfo.cpp
    WindowManager.hpp
)
if(WIN32)
    list(APPEND DISPLAY_SOURCE_FILES Windows/Window.cpp)
    list(APPEND DISPLAY_SOURCE_FILES Windows/WindowManager.cpp)
endif()

add_library(ChiaDisplay STATIC ${DISPLAY_SOURCE_FILES})
target_include_directories(ChiaDisplay PUBLIC ${INCLUDE_DIR})
if (DIRECTX_ENABLED)
    target_link_libraries(ChiaDisplay PUBLIC ChiaDX)
elseif(VULKAN_ENABLED)
    target_link_libraries(ChiaDisplay PUBLIC ChiaVulkan)
else()
    target_link_libraries(ChiaDisplay PUBLIC ChiaOpenGL)
endif()
