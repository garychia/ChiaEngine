set(DIRECTX_SOURCE_FILES
    Constants.hpp
    DirectXHelper.hpp
    DirectXHelper.cpp
    DirectXRenderer.hpp
    DirectXRenderer.cpp)

add_library(ChiaDX STATIC ${DIRECTX_SOURCE_FILES})
target_include_directories(ChiaDX PUBLIC ${INCLUDE_DIR})
target_link_libraries(ChiaDX PUBLIC
    d3d11.lib
    dxgi.lib
    d3dcompiler.lib
    Windowscodecs.lib
)
