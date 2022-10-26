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
    GUI/Border.hpp
    GUI/Border.cpp
    GUI/Button.hpp
    GUI/Button.cpp
    GUI/GUILayer.hpp
    GUI/GUILayer.cpp
    GUI/GUILayout.hpp
    GUI/GUILayout.cpp
    GUI/HorizontalList.hpp
    GUI/HorizontalList.cpp
    GUI/IGUI.hpp
    GUI/IGUI.cpp
    GUI/IInteractable.hpp
    GUI/IInteractable.cpp
)
if(DIRECTX_ENABLED)
    list(APPEND DISPLAY_SOURCE_FILES Windows/Window.cpp)
    list(APPEND DISPLAY_SOURCE_FILES Windows/WindowManager.cpp)
elseif(VULKAN_ENABLED)
    list(APPEND DISPLAY_SOURCE_FILES GLFW/Window.cpp)
    list(APPEND DISPLAY_SOURCE_FILES GLFW/WindowManager.cpp)
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
