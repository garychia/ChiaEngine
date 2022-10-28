set(CHIA_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/src/include)
if(NOT DIRECTX_ENABLED)
    list(APPEND CHIA_INCLUDE_DIRS ${glfw_SOURCE_DIR}/include)
    list(APPEND CHIA_INCLUDE_DIRS ${glm_SOURCE_DIR})
endif()
if(VULKAN_ENABLED)
    list(APPEND CHIA_INCLUDE_DIRS ${Vulkan_INCLUDE_DIRS})
endif()

set(APP_SOURCE_FILES include/App/App.hpp)
if(DIRECTX_ENABLED)
    list(APPEND APP_SOURCE_FILES source/App/DirectX/App.cpp)
elseif(VULKAN_ENABLED)
    list(APPEND APP_SOURCE_FILES source/App/Vulkan/App.cpp)
endif()

add_subdirectory(include/Data)

set(DISPLAY_SOURCE_FILES
    include/Display/Camera.hpp
    source/Display/Camera.cpp
    include/Display/Color.hpp
    source/Display/Color.cpp
    include/Display/IRenderable.hpp
    source/Display/IRenderable.cpp
    include/Display/IRenderer.hpp
    include/Display/Renderer.hpp
    source/Display/Renderer.cpp
    include/Display/Scene.hpp
    source/Display/Scene.cpp
    include/Display/Shader.hpp
    source/Display/Shader.cpp
    include/Display/Texture.hpp
    source/Display/Texture.cpp
    include/Display/Window.hpp
    include/Display/WindowInfo.hpp
    source/Display/WindowInfo.cpp
    include/Display/WindowManager.hpp
    include/Display/GUI/Border.hpp
    source/Display/GUI/Border.cpp
    include/Display/GUI/Button.hpp
    source/Display/GUI/Button.cpp
    include/Display/GUI/GUILayer.hpp
    source/Display/GUI/GUILayer.cpp
    include/Display/GUI/GUILayout.hpp
    source/Display/GUI/GUILayout.cpp
    include/Display/GUI/HorizontalList.hpp
    source/Display/GUI/HorizontalList.cpp
    include/Display/GUI/IGUI.hpp
    source/Display/GUI/IGUI.cpp
    include/Display/GUI/IInteractable.hpp
    source/Display/GUI/IInteractable.cpp
)
if (DIRECTX_ENABLED)
    list(APPEND DISPLAY_SOURCE_FILES
        include/Display/DirectX/Constants.hpp
        include/Display/DirectX/DirectXHelper.hpp
        source/Display/DirectX/DirectXHelper.cpp
        include/Display/DirectX/DirectXRenderer.hpp
        source/Display/DirectX/DirectXRenderer.cpp
        source/Display/Windows/Window.cpp
        source/Display/Windows/WindowManager.cpp
    )
elseif(VULKAN_ENABLED)
    list(APPEND DISPLAY_SOURCE_FILES
        include/Display/GLFW/Window.hpp
        source/Display/GLFW/Window.cpp
        source/Display/GLFW/WindowManager.cpp
        include/Display/VulkanHelper.hpp
        source/Display/VulkanHelper.cpp
    )
else()
    # TODO: Support OpenGL
    list(APPEND DISPLAY_SOURCE_FILES
        include/Display/GLFW/Window.hpp
        source/Display/GLFW/Window.cpp
        source/Display/GLFW/WindowManager.cpp
    )
endif()

set(GEOMETRY_SOURCE_FILES
    include/Geometry/2D/Point2D.hpp
    source/Geometry/2D/Point2D.cpp
    include/Geometry/3D/Point3D.hpp
    source/Geometry/3D/Point3D.cpp
    include/Geometry/Primitives.hpp
    source/Geometry/Primitives.cpp
)

set(MATH_SOURCE_FILES
    include/Math/Math.hpp
    source/Math/Math.cpp
)

add_subdirectory(include/System/Operation)

set(SYSTEM_SOURCE_FILES
    include/System/Debug/Debug.hpp
    include/System/Input/InputHandleLayer.hpp
    source/System/Input/InputHandleLayer.cpp
    include/System/Input/InputHandler.hpp
    source/System/Input/InputHandler.cpp
    include/System/Input/KeyboardHandler.hpp
    source/System/Input/KeyboardHandler.cpp
    include/System/Input/KeyCodes.hpp
    include/System/Input/keyCombination.hpp
    source/System/Input/keyCombination.cpp
    include/System/Input/MouseInfo.hpp
    source/System/Input/MouseInfo.cpp
    include/System/Input/MouseInput.hpp
    source/System/Input/MouseInput.cpp
    include/System/IO/IO.hpp
    source/System/IO/IO.cpp
)

if(WIN32)
    list(APPEND SYSTEM_SOURCE_FILES source/System/Debug/Windows/Debug.cpp)
    list(APPEND SYSTEM_SOURCE_FILES include/System/Input/Windows/WinKeyCode.hpp)
endif()

set(CHIA_ENGINE_SOURCE_FILES
    ${APP_SOURCE_FILES}
    ${DISPLAY_SOURCE_FILES}
    ${GEOMETRY_SOURCE_FILES}
    ${MATH_SOURCE_FILES}
    ${SYSTEM_SOURCE_FILES}
    include/pch.hpp
    include/Paths.hpp
    include/Version.hpp
)

add_library(ChiaEngineLib STATIC ${CHIA_ENGINE_SOURCE_FILES})
target_include_directories(ChiaEngineLib PUBLIC ${CHIA_INCLUDE_DIRS})
target_precompile_headers(ChiaEngineLib PUBLIC include/pch.hpp)
if(DIRECTX_ENABLED)
    target_link_libraries(ChiaEngineLib
        d3d11.lib
        dxgi.lib
        d3dcompiler.lib
        Windowscodecs.lib
    )
elseif(VULKAN_ENABLED)
    target_link_libraries(ChiaEngineLib "Vulkan::Vulkan")
endif()
