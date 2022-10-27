set(CHIA_APP_SOURCE_FILES
    Components/PanelLayout.hpp
    Components/PanelLayout.cpp
    Components/TopPanelBar.hpp
    Components/TopPanelBar.cpp
    ChiaApp.hpp
    ChiaApp.cpp
    Panel.hpp
    Panel.cpp
    SceneWindow.hpp
    SceneWindow.cpp
)

if(WIN32)
    add_executable(ChiaApp WIN32 ${CHIA_APP_SOURCE_FILES} WinMain.cpp)
else()
    add_executable(ChiaApp ${CHIA_APP_SOURCE_FILES} main.cpp)
endif()

target_link_libraries(ChiaApp ChiaEngineLib)
target_include_directories(ChiaApp PUBLIC ${CHIA_INCLUDE_DIRS})
