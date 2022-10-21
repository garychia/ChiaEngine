#ifndef DIRECTX_HELPER_HPP
#define DIRECTX_HELPER_HPP

#include "pch.hpp"

using namespace Microsoft::WRL;

class DirectXHelper
{
  public:
    static bool CreateDevice(ComPtr<ID3D11Device> &pDevice, ComPtr<ID3D11DeviceContext> &pContext,
                             D3D_FEATURE_LEVEL &featureLevel);

    static bool CreateSwapChain(HWND windowHandle, bool fullScreen, ComPtr<IDXGISwapChain> &pSwapChain,
                                ComPtr<ID3D11Device> &pDevice, UINT nBuffers = 2);

    static bool CreateRenderTarget(ComPtr<ID3D11Device> &pDevice, ComPtr<IDXGISwapChain> &pSwapChain,
                                   ComPtr<ID3D11Texture2D> &pBackBuffer, ComPtr<ID3D11RenderTargetView> &pRenderTarget,
                                   D3D11_TEXTURE2D_DESC &backBufferDesc);

    static bool CreateDepthStencilBuffer(ComPtr<ID3D11Device> &pDevice, CD3D11_TEXTURE2D_DESC &desc,
                                         ComPtr<ID3D11Texture2D> &pDepthStencil,
                                         ComPtr<ID3D11DepthStencilView> &pDepthStencilView);

    static bool CreateViewport(D3D11_VIEWPORT &viewport, D3D11_TEXTURE2D_DESC &backBufferDesc,
                               ComPtr<ID3D11DeviceContext> &pDeviceContext);

    static bool LoadVertexShader(ComPtr<ID3D11Device> &pDevice, LPVOID byteCode, size_t codeSize,
                                 ComPtr<ID3D11VertexShader> &pVertexShader, const D3D11_INPUT_ELEMENT_DESC *inputDescs,
                                 size_t nInputDescs, ComPtr<ID3D11InputLayout> &pInputLayout);

    static bool LoadPixelShader(ComPtr<ID3D11Device> &pDevice, LPVOID byteCode, size_t codeSize,
                                ComPtr<ID3D11PixelShader> &pPixelShader);
};

#endif
