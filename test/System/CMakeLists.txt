add_subdirectory(IO)
set(SYSTEM_TEST_SOURCE_FILES
    SystemModule.hpp
    SystemModule.cpp
    # ADR-0001 D3:DockRegistryTest 直接編譯 dock manager(headless-TU),
    # 驗證 named docks 定位 / collapse(不經 GLFW Window)。
    ../../src/ChiaApp/Components/PanelLayout.cpp
    ../../src/ChiaApp/Components/TopPanelBar.cpp
)
add_library(SystemTest STATIC ${SYSTEM_TEST_SOURCE_FILES})
target_include_directories(SystemTest PUBLIC
    ${CMAKE_SOURCE_DIR}/test
    ${CMAKE_SOURCE_DIR}/src/include
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/include>
)
# JobSystem/AssetManager 用 std::thread (SystemModule → AssetTest)
find_package(Threads REQUIRED)
target_link_libraries(SystemTest PUBLIC IOTest Threads::Threads)
