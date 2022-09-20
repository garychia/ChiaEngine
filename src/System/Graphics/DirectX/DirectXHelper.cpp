#include "DirectXHelper.hpp"

#include "Constants.hpp"
#include "Debug/Debug.hpp"

bool DirectXHelper::CreateDevice(ComPtr<ID3D11Device> &pDevice, ComPtr<ID3D11DeviceContext> &pContext, D3D_FEATURE_LEVEL &featureLevel)
{
    HRESULT hr;
    UINT deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    ComPtr<ID3D11Device> pDevice;
    ComPtr<ID3D11DeviceContext> ppContext;

#if defined(DEBUG)
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    hr = D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        0,
        deviceFlags,
        DX_SUPPORTED_LEVELS,
        ARRAYSIZE(DX_SUPPORTED_LEVELS),
        D3D11_SDK_VERSION,
        &pDevice,
        &featureLevel,
        &pContext);
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXHelper: Failed to create a ID3D11Device.");
        return false;
    }
    return true;
}

bool DirectXHelper::CreateSwapChain(HWND windowHandle, bool fullScreen, ComPtr<IDXGISwapChain> &pSwapChain, ComPtr<IDXGIDevice> &pDevice, UINT nBuffers)
{
    HRESULT hr;
    DXGI_SWAP_CHAIN_DESC desc;

    ZeroMemory(&desc, sizeof(DXGI_SWAP_CHAIN_DESC));
    desc.Windowed = fullScreen ? FALSE : TRUE;
    desc.BufferCount = nBuffers;
    desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.OutputWindow = windowHandle;

    ComPtr<IDXGIDevice3> dxgiDevice;
    hr = pDevice.As(&dxgiDevice);
    if (FAILED(hr))
    {
        PRINT_ERR("DirectXHelper: Failed to create a IDXGIDevice3.")
        return false;
    }

    ComPtr<IDXGIAdapter> pAdapter;
    ComPtr<IDXGIFactory> pFactory;
    hr = dxgiDevice->GetAdapter(&pAdapter);
    if (FAILED(hr))
    {
        PRINT_ERR("DirectXHelper: Failed to retrieve the adapter.")
        return false;
    }

    pAdapter->GetParent(IID_PPV_ARGS(&pFactory));
    hr = pFactory->CreateSwapChain(
        pDevice.Get(),
        &desc,
        &pSwapChain);
    if (FAILED(hr))
    {
        PRINT_ERR("DirectXHelper: Failed to create the swap chain.")
        return false;
    }
    return true;
}

bool DirectXHelper::CreateRenderTarget(ComPtr<ID3D11Device> &pDevice, ComPtr<IDXGISwapChain> &pSwapChain, ComPtr<ID3D11Texture2D> &pBackBuffer, ComPtr<ID3D11RenderTargetView> &pRenderTarget, D3D11_TEXTURE2D_DESC &backBufferDesc)
{
    HRESULT hr = pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), &pBackBuffer);
    if (FAILED(hr))
    {
        PRINT_ERR("DirectXHelper: Failed to get the back buffer.");
        return false;
    }
    hr = pDevice->CreateRenderTargetView(
        pBackBuffer.Get(),
        nullptr,
        pRenderTarget.GetAddressOf());
    if (FAILED(hr))
    {
        PRINT_ERR("DirectXHelper: Failed to create a render target.");
        return false;
    }
    pBackBuffer->GetDesc(&backBufferDesc);
    if (FAILED(hr))
    {
        PRINT_ERR("DirectXHelper: Failed to retrieve the desc of back buffer.");
        return false;
    }
    return true;
}

bool DirectXHelper::CreateDepthStencilBuffer(ComPtr<ID3D11Device> &pDevice, CD3D11_TEXTURE2D_DESC &desc, ComPtr<ID3D11Texture2D> &pDepthStencil, ComPtr<ID3D11DepthStencilView> &pDepthStencilView)
{
    HRESULT hr = pDevice->CreateTexture2D(&desc, nullptr, &pDepthStencil);
    if (FAILED(hr))
    {
        PRINT_ERR("DirectXHelper: Failed to create a depth-stencil buffer.")
        return false;
    }
    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    hr = pDevice->CreateDepthStencilView(pDepthStencil.Get(), &depthStencilViewDesc, &pDepthStencilView);
    if (FAILED(hr))
    {
        PRINT_ERR("DirectXHelper: Failed to create a depth-stencil view.");
        return false;
    }
    return true;
}

bool DirectXHelper::CreateViewport(D3D11_VIEWPORT &viewport, D3D11_TEXTURE2D_DESC &backBufferDesc, ComPtr<ID3D11DeviceContext> &pDeviceContext)
{
    ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));
    viewport.Height = (FLOAT)backBufferDesc.Height;
    viewport.Width = (FLOAT)backBufferDesc.Width;
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;
    pDeviceContext->RSSetViewports(1, &viewport);
}
