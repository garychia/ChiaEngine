#ifndef DIRECTX_HELPER_HPP
#define DIRECTX_HELPER_HPP

#include <wrl\client.h>
#include <d3d11.h>
#include <dxgi1_3.h>

using namespace Microsoft::WRL;

class DirectXHelper
{
public:
    static bool CreateDevice(ComPtr<ID3D11Device> &pDevice, ComPtr<ID3D11DeviceContext> &pContext, D3D_FEATURE_LEVEL &featureLevel);

    static bool CreateSwapChain(HWND windowHandle, bool fullScreen, ComPtr<IDXGISwapChain> &pSwapChain, ComPtr<IDXGIDevice> &pDevice, UINT nBuffers = 2);

    static bool CreateRenderTarget(ComPtr<ID3D11Device> &pDevice, ComPtr<IDXGISwapChain> &pSwapChain, ComPtr<ID3D11Texture2D> &pBackBuffer, ComPtr<ID3D11RenderTargetView> &pRenderTarget, D3D11_TEXTURE2D_DESC &backBufferDesc);

    static bool CreateDepthStencilBuffer(ComPtr<ID3D11Device> &pDevice, CD3D11_TEXTURE2D_DESC &desc, ComPtr<ID3D11Texture2D> &pDepthStencil, ComPtr<ID3D11DepthStencilView> &pDepthStencilView);

    static bool CreateViewport(D3D11_VIEWPORT &viewport, D3D11_TEXTURE2D_DESC &backBufferDesc, ComPtr<ID3D11DeviceContext> &pDeviceContext);
};

#endif
