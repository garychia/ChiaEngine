#ifndef DIRECTX_HELPER_HPP
#define DIRECTX_HELPER_HPP

#include "pch.hpp"

class DirectXHelper
{
  public:
    static bool CreateDevice(Microsoft::WRL::ComPtr<ID3D11Device> &pDevice, Microsoft::WRL::ComPtr<ID3D11DeviceContext> &pContext,
                             D3D_FEATURE_LEVEL &featureLevel);

    static bool CreateSwapChain(HWND windowHandle, bool fullScreen, Microsoft::WRL::ComPtr<IDXGISwapChain> &pSwapChain,
                                Microsoft::WRL::ComPtr<ID3D11Device> &pDevice, UINT nBuffers = 2);

    static bool CreateRenderTarget(Microsoft::WRL::ComPtr<ID3D11Device> &pDevice, Microsoft::WRL::ComPtr<IDXGISwapChain> &pSwapChain,
                                   Microsoft::WRL::ComPtr<ID3D11Texture2D> &pBackBuffer, Microsoft::WRL::ComPtr<ID3D11RenderTargetView> &pRenderTarget,
                                   D3D11_TEXTURE2D_DESC &backBufferDesc);

    static bool CreateDepthStencilBuffer(Microsoft::WRL::ComPtr<ID3D11Device> &pDevice, CD3D11_TEXTURE2D_DESC &desc,
                                         Microsoft::WRL::ComPtr<ID3D11Texture2D> &pDepthStencil,
                                         Microsoft::WRL::ComPtr<ID3D11DepthStencilView> &pDepthStencilView);

    static bool CreateViewport(D3D11_VIEWPORT &viewport, D3D11_TEXTURE2D_DESC &backBufferDesc,
                               Microsoft::WRL::ComPtr<ID3D11DeviceContext> &pDeviceContext);

    static bool LoadVertexShader(Microsoft::WRL::ComPtr<ID3D11Device> &pDevice, LPVOID byteCode, size_t codeSize,
                                 Microsoft::WRL::ComPtr<ID3D11VertexShader> &pVertexShader, const D3D11_INPUT_ELEMENT_DESC *inputDescs,
                                 size_t nInputDescs, Microsoft::WRL::ComPtr<ID3D11InputLayout> &pInputLayout);

    static bool LoadPixelShader(Microsoft::WRL::ComPtr<ID3D11Device> &pDevice, LPVOID byteCode, size_t codeSize,
                                Microsoft::WRL::ComPtr<ID3D11PixelShader> &pPixelShader);
};

#endif