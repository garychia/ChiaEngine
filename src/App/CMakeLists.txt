set(APP_SOURCE_FILES
    App.hpp
    Panel.hpp
    Panel.cpp
    SceneWindow.hpp
    SceneWindow.cpp
    Components/TopPanelBar.hpp
    Components/TopPanelBar.cpp
)

if(DIRECTX_ENABLED)
    list(APPEND APP_SOURCE_FILES DirectX/App.cpp)
elseif(VULKAN_ENABLED)
    list(APPEND APP_SOURCE_FILES Vulkan/App.cpp)
endif()

add_library(ChiaApp STATIC ${APP_SOURCE_FILES})
target_include_directories(ChiaApp PUBLIC
    ${INCLUDE_DIR}
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
)
target_link_libraries(ChiaApp PUBLIC
    ChiaData
    ChiaSystem
    ChiaDisplay
)
