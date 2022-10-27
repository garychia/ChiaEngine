#include "Display/DirectX/DirectXRenderer.hpp"
#include "System/IO/IO.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "3rdparty/stb_image.h"

const char DirectXRenderer::DefaultVertexShader[] =
    "cbuffer GlobalBuffer : register(b0)"
    "{"
    "   matrix worldMatrix;"
    "   matrix viewMatrix;"
    "   matrix projectionMatrix;"
    "};"
    "struct Vertex"
    "{"
    "   float3 position : POSITION;"
    "   float4 color : COLOR;"
    "   float2 texcoord : TEXCOORD;"
    "   uint cmode : COLORMODE;"
    "   uint gui : GUIFLAG;"
    "};"
    "struct Pixel"
    "{"
    "   float4 position : SV_POSITION;"
    "   float4 color : COLOR;"
    "   float2 texcoord : TEXCOORD;"
    "   uint cmode : COLORMODE;"
    "};"
    "Pixel vsMain(Vertex vertex)"
    "{"
    "   Pixel pixel;"
    "   float4 position = {vertex.position.xyz, 1};"
    "   if (vertex.gui)"
    "      pixel.position = mul(position, worldMatrix);"
    "   else"
    "      pixel.position = mul(mul(mul(position, worldMatrix), viewMatrix), projectionMatrix);"
    "   pixel.color = vertex.color;"
    "   pixel.texcoord = vertex.texcoord;"
    "   pixel.cmode = vertex.cmode;"
    "   return pixel;"
    "}";

const char DirectXRenderer::DefaultPixelShader[] = "struct Pixel"
                                                   "{"
                                                   "    float4 position : SV_POSITION;"
                                                   "    float4 color : COLOR;"
                                                   "    float2 texcoord : TEXCOORD;"
                                                   "    uint cmode : COLORMODE;"
                                                   "};"
                                                   "Texture2D text;"
                                                   "SamplerState samplerState;"
                                                   "float4 psMain(Pixel pixel) : SV_TARGET"
                                                   "{"
                                                   "    if (pixel.cmode == 0)"
                                                   "        return pixel.color;"
                                                   "    return text.Sample(samplerState, pixel.texcoord);"
                                                   "}";

const D3D11_INPUT_ELEMENT_DESC DirectXRenderer::InputDescs[] = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLORMODE", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"GUIFLAG", 0, DXGI_FORMAT_R32_UINT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0}};

DirectXRenderer::DirectXRenderer()
    : backBufferDesc(), featureLevel(), matrixBuffer(), viewport(), pVertexShaders(), pPixelShaders(), pCamera()
{
    UpdateConstantBuffer();
}

DirectXRenderer::~DirectXRenderer()
{
}

void DirectXRenderer::UpdateConstantBuffer()
{
    if (!pCamera)
    {
        DirectX::XMStoreFloat4x4(&matrixBuffer.world, DirectX::XMMatrixIdentity());
        DirectX::XMStoreFloat4x4(&matrixBuffer.view, DirectX::XMMatrixIdentity());
        DirectX::XMStoreFloat4x4(&matrixBuffer.projection, DirectX::XMMatrixIdentity());
    }
    else
    {
        OnCameraChanged();
    }
}

bool DirectXRenderer::SetupBackBuffer()
{
    if (!DirectXHelper::CreateRenderTarget(pDevice, pSwapChain, pBackBuffer, pRenderTarget, backBufferDesc))
    {
        PRINTLN_ERR("DirectXRenderer: failed to set up a render target for the window.");
        return false;
    }
    CD3D11_TEXTURE2D_DESC depthStencilDesc(DXGI_FORMAT_D24_UNORM_S8_UINT, backBufferDesc.Width, backBufferDesc.Height,
                                           1, 1, D3D11_BIND_DEPTH_STENCIL);
    if (!DirectXHelper::CreateDepthStencilBuffer(pDevice, depthStencilDesc, pDepthStencil, pDepthStencilView))
    {
        PRINTLN_ERR("DirectXRenderer: failed to set up a depth stencil buffer for the window.");
        return false;
    }
    if (!DirectXHelper::CreateViewport(viewport, backBufferDesc, pContext))
    {
        PRINTLN_ERR("DirectXRenderer: failed to set up a viewport for the window.");
        return false;
    }
    return true;
}

void DirectXRenderer::ReleaseBackBuffer()
{
    pRenderTarget.Reset();
    pBackBuffer.Reset();
    pDepthStencilView.Reset();
    pDepthStencil.Reset();
    pContext->Flush();
}

bool DirectXRenderer::LoadVertexShaderFromFile(const String &path, ComPtr<ID3D11VertexShader> &pVertexShader)
{
    const size_t bufferSize = 4196;
    char *buffer = new char[bufferSize];
    FileIO vertexFile(path);
    if (!vertexFile.Open())
    {
        PRINTLN_ERR("DirectXRenderer: failed to open the file of vertex shader.");
        delete[] buffer;
        return false;
    }
    const auto nBytesRead = vertexFile.ReadBytes(bufferSize, buffer);
    vertexFile.Close();
    if (!nBytesRead)
    {
        PRINTLN_ERR("DirectXRenderer: the file of vertex shader is empty.");
        delete[] buffer;
        return false;
    }
    ComPtr<ID3DBlob> pCompiledBuffer;
    ComPtr<ID3DBlob> pError;
    UINT compileFlag = 0;
#ifdef DEBUG
    compileFlag |= D3DCOMPILE_DEBUG;
#endif
    HRESULT hr = D3DCompile(buffer, nBytesRead, "CustomVertexShader", NULL, NULL, "vsMain", "vs_5_0", compileFlag, 0,
                            pCompiledBuffer.GetAddressOf(), pError.GetAddressOf());
    delete[] buffer;
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: failed to compile the given vertex shader.");

        return false;
    }
    auto result =
        DirectXHelper::LoadVertexShader(pDevice, pCompiledBuffer->GetBufferPointer(), pCompiledBuffer->GetBufferSize(),
                                        pVertexShader, InputDescs, ARRAYSIZE(InputDescs), pInputLayout);
    if (!result)
        PRINTLN_ERR("DirectXRenderer: the file of vertex shader is empty.");
    return result;
}

bool DirectXRenderer::LoadPixelShaderFromFile(const String &path, ComPtr<ID3D11PixelShader> &pPixelShader)
{
    const size_t bufferSize = 4196;
    char *buffer = new char[bufferSize];
    FileIO pixelFile(path);
    if (!pixelFile.Open())
    {
        PRINTLN_ERR("DirectXRenderer: failed to open the file of pixel shader.");
        delete[] buffer;
        return false;
    }
    const auto nBytesRead = pixelFile.ReadBytes(bufferSize, buffer);
    pixelFile.Close();
    if (!nBytesRead)
    {
        PRINTLN_ERR("DirectXRenderer: the file of pixel shader is empty.");
        delete[] buffer;
        return false;
    }
    ComPtr<ID3DBlob> pCompiledBuffer;
    ComPtr<ID3DBlob> pError;
    UINT compileFlag = 0;
#ifdef DEBUG
    compileFlag |= D3DCOMPILE_DEBUG;
#endif
    HRESULT hr = D3DCompile(DefaultPixelShader, sizeof(DefaultPixelShader), "CustomPixelShader", NULL, NULL, "psMain",
                            "vs_5_0", compileFlag, 0, pCompiledBuffer.GetAddressOf(), 0);
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: failed to compile the default pixel shader.");
        return false;
    }
    auto result = DirectXHelper::LoadPixelShader(pDevice, pCompiledBuffer->GetBufferPointer(),
                                                 pCompiledBuffer->GetBufferSize(), pPixelShader);
    if (!result)
        PRINTLN_ERR("DirectXRenderer: the file of pixel shader is empty.");
    delete[] buffer;
    return result;
}

bool DirectXRenderer::LoadDefaultVertexShader()
{
    ComPtr<ID3DBlob> pCompiledBuffer;
    ComPtr<ID3DBlob> pError;
    UINT compileFlag = 0;
#ifndef NDEBUG
    compileFlag |= D3DCOMPILE_DEBUG;
#endif
    HRESULT hr = D3DCompile(DefaultVertexShader, sizeof(DefaultVertexShader), "DefaultVertexShader", NULL, NULL,
                            "vsMain", "vs_5_0", compileFlag, 0, pCompiledBuffer.GetAddressOf(), pError.GetAddressOf());
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: failed to compile the default vertex shader.");
        return false;
    }
    hr = pDevice->CreateVertexShader(pCompiledBuffer->GetBufferPointer(), pCompiledBuffer->GetBufferSize(), nullptr,
                                     pDefaultVertexShader.GetAddressOf());
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: failed to create the default vertex shader.");
        return false;
    }
    hr = pDevice->CreateInputLayout(InputDescs, ARRAYSIZE(InputDescs), pCompiledBuffer->GetBufferPointer(),
                                    pCompiledBuffer->GetBufferSize(), pInputLayout.GetAddressOf());
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: failed to create the input layout.");
        return false;
    }
    return true;
}

bool DirectXRenderer::LoadDefaultPixelShader()
{
    ComPtr<ID3DBlob> pCompiledBuffer;
    ComPtr<ID3DBlob> pError;
    UINT compileFlag = 0;
#ifndef NDEBUG
    compileFlag |= D3DCOMPILE_DEBUG;
#endif
    HRESULT hr = D3DCompile(DefaultPixelShader, sizeof(DefaultPixelShader), "DefaultPixelShader", NULL, NULL, "psMain",
                            "ps_5_0", compileFlag, 0, pCompiledBuffer.GetAddressOf(), pError.GetAddressOf());
    if (FAILED(hr))
    {
        auto message = (char *)pError->GetBufferPointer();
        PRINTLN_ERR("DirectXRenderer: failed to compile the default pixel shader.");
        return false;
    }
    hr = pDevice->CreatePixelShader(pCompiledBuffer->GetBufferPointer(), pCompiledBuffer->GetBufferSize(), nullptr,
                                    pDefaultPixelShader.GetAddressOf());
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: Failed to create the default pixel shader.");
        return false;
    }
    return true;
}

bool DirectXRenderer::Initialize(HWND windowHandle, bool fullScreen)
{
    if (!DirectXHelper::CreateDevice(pDevice, pContext, featureLevel))
        return false;

    D3D11_RASTERIZER_DESC rasterizerDesc;
    ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));
    rasterizerDesc.FillMode = D3D11_FILL_SOLID;
    rasterizerDesc.CullMode = D3D11_CULL_BACK;
    rasterizerDesc.FrontCounterClockwise = true;
    rasterizerDesc.DepthBias = false;
    rasterizerDesc.DepthBiasClamp = 0;
    rasterizerDesc.SlopeScaledDepthBias = 0;
    rasterizerDesc.DepthClipEnable = true;
    rasterizerDesc.ScissorEnable = false;
    rasterizerDesc.MultisampleEnable = true;
    rasterizerDesc.AntialiasedLineEnable = true;
    HRESULT hr = pDevice->CreateRasterizerState(&rasterizerDesc, pRasterizerState.GetAddressOf());
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: Failed to configure the rasterization stage.");
        return false;
    }
    pContext->RSSetState(pRasterizerState.Get());

    if (!InitializeTextureSampleState() || !CompileDefaultShaders() ||
        !DirectXHelper::CreateSwapChain(windowHandle, fullScreen, pSwapChain, pDevice))
        return false;
    return SetupBackBuffer();
}

bool DirectXRenderer::CompileDefaultShaders()
{
    if (!LoadDefaultVertexShader())
        return false;
    if (!LoadDefaultPixelShader())
        return false;

    CD3D11_BUFFER_DESC cbufferDesc(sizeof(MatrixBuffer), D3D11_BIND_CONSTANT_BUFFER);
    HRESULT hr = pDevice->CreateBuffer(&cbufferDesc, nullptr, pMatrixBuffer.GetAddressOf());
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: failed to create a buffer for transformation matrices.");
        return false;
    }
    return true;
}

bool DirectXRenderer::InitializeTextureSampleState()
{
    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    HRESULT hr = pDevice->CreateSamplerState(&samplerDesc, pSamplerState.GetAddressOf());
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: failed to create a sampler state.");
        return false;
    }
    return true;
}

bool DirectXRenderer::LoadTexture(Texture *pTexture)
{
    // Load the texture image file.
    const auto &imagePath = pTexture->imagePath;
    const auto charPathSize = (imagePath.Length() << 1) + 1;
    char *imagePathCharPath = new char[charPathSize];
    imagePath.ToUTF8(imagePathCharPath, charPathSize);
    int width, height, bitsPerPixel;
    stbi_set_flip_vertically_on_load(1);
    float *imageData = stbi_loadf(imagePathCharPath, &width, &height, &bitsPerPixel, 4);
    delete[] imagePathCharPath;
    if (!imageData)
    {
        PRINTLN_ERR("DirectXRenderer: failed to open the texture image.");
        return false;
    }

    const auto stride = width * 4 * sizeof(float);
    const DXGI_FORMAT imageFormat = DXGI_FORMAT_R32G32B32A32_FLOAT;
    D3D11_TEXTURE2D_DESC textureDesc;
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = imageFormat;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DEFAULT;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = 0;
    textureDesc.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA resourceData;
    resourceData.pSysMem = imageData;
    resourceData.SysMemPitch = static_cast<UINT>(stride);
    resourceData.SysMemSlicePitch = static_cast<UINT>(height);
    const auto textureID = pTextures.Length();
    ComPtr<ID3D11Texture2D> pDXTexture2D;
    HRESULT hr = pDevice->CreateTexture2D(&textureDesc, &resourceData, pDXTexture2D.GetAddressOf());
    stbi_image_free(imageData);
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: failed to create a Texture2D.");
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResouceViewDesc;
    ZeroMemory(&shaderResouceViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
    shaderResouceViewDesc.Format = imageFormat;
    shaderResouceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResouceViewDesc.Texture2D.MipLevels = 1;
    ComPtr<ID3D11ShaderResourceView> pDXShaderResourceView;
    hr = pDevice->CreateShaderResourceView(pDXTexture2D.Get(), &shaderResouceViewDesc,
                                           pDXShaderResourceView.GetAddressOf());
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: failed to create a ShaderResourceView.");
        return false;
    }
    pTextures.Append(pDXTexture2D);
    pShaderResourceViews.Append(pDXShaderResourceView);
    pTexture->loaded = true;
    pTexture->identifier = textureID;
    return true;
}

bool DirectXRenderer::LoadRenderable(IRenderable &renderable, Scene::SceneType sceneType)
{
    if (renderable.IsLoaded())
        return true;

    RenderInfo renderInfo = renderable.GetRenderInfo();
    // Load the texture if provided.
    if (renderInfo.pTexture && !renderInfo.pTexture->loaded && !LoadTexture(renderInfo.pTexture))
        return false;

    // Create the input buffer.
    VertexInfo *inputBuffer = CreateInputBuffer(renderable, sceneType);
    if (!inputBuffer)
        return false;

    CD3D11_BUFFER_DESC vDesc(sizeof(VertexInfo) * renderInfo.numOfVertices, D3D11_BIND_VERTEX_BUFFER);
    D3D11_SUBRESOURCE_DATA vertexData;
    ZeroMemory(&vertexData, sizeof(D3D11_SUBRESOURCE_DATA));
    vertexData.pSysMem = inputBuffer;
    vertexData.SysMemPitch = 0;
    vertexData.SysMemSlicePitch = 0;
    ComPtr<ID3D11Buffer> pVertexBuffer;
    HRESULT hr = pDevice->CreateBuffer(&vDesc, &vertexData, pVertexBuffer.GetAddressOf());
    delete[] inputBuffer;
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: failed to create a vertex buffer.");
        return false;
    }

    // Prepare the vertex index buffer.
    const auto indexBuffer = renderInfo.vertexIndexBuffer;
    const auto nIndices = renderInfo.numOfVertexIndices;
    if (!nIndices || !indexBuffer)
        return false;
    CD3D11_BUFFER_DESC indexDesc(sizeof(unsigned short) * nIndices, D3D11_BIND_INDEX_BUFFER);
    D3D11_SUBRESOURCE_DATA indexData;
    ZeroMemory(&indexData, sizeof(D3D11_SUBRESOURCE_DATA));
    indexData.pSysMem = indexBuffer;
    indexData.SysMemPitch = 0;
    indexData.SysMemSlicePitch = 0;
    ComPtr<ID3D11Buffer> pIndexBuffer;
    hr = pDevice->CreateBuffer(&indexDesc, &indexData, pIndexBuffer.GetAddressOf());
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: failed to create an index buffer.");
        return false;
    }

    // Generate an ID for the renderable.
    renderable.MarkLoaded(pIndexBuffers.Length());
    pVertexBuffers.Append(pVertexBuffer);
    pIndexBuffers.Append(pIndexBuffer);
    return true;
}

void DirectXRenderer::RenderRenderable(IRenderable &renderable)
{
    if (!renderable.IsLoaded())
        return;

    UINT stride = sizeof(VertexInfo);
    UINT offset = 0;
    
    RenderInfo renderInfo = renderable.GetRenderInfo();
    const auto objectID = renderable.GetIdentifier();
    const auto &position = renderable.GetPosition();
    const auto &rotation = renderable.GetRotation();
    const auto &scale = renderable.GetScale();
    const auto ptrVertexShader = renderInfo.pVertexShader;
    const auto ptrPixelShader = renderInfo.pPixelShader;
    const auto ptrTexture = renderInfo.pTexture;

    DirectX::XMStoreFloat4x4(
        &matrixBuffer.world,
        DirectX::XMMatrixTranspose(DirectX::XMMatrixScaling(scale.x, scale.y, scale.z) *
                                   DirectX::XMMatrixRotationRollPitchYaw(DirectX::XMConvertToRadians(-rotation.x),
                                                                         DirectX::XMConvertToRadians(-rotation.y),
                                                                         DirectX::XMConvertToRadians(-rotation.z)) *
                                   DirectX::XMMatrixTranslation(position.x, position.y, -position.z)));
    // Update the constant buffer (transformation matrices).
    pContext->UpdateSubresource(pMatrixBuffer.Get(), 0, nullptr, &matrixBuffer, 0, 0);
    // Setup the vertex and the index buffer.
    pContext->IASetVertexBuffers(0, 1, pVertexBuffers[objectID].GetAddressOf(), &stride, &offset);
    pContext->IASetIndexBuffer(pIndexBuffers[objectID].Get(), DXGI_FORMAT_R16_UINT, 0);
    pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pContext->IASetInputLayout(pInputLayout.Get());

    if (ptrTexture && ptrTexture->loaded)
    {
        const auto textureID = ptrTexture->identifier;
        pContext->PSSetShaderResources(0, 1, pShaderResourceViews[textureID].GetAddressOf());
    }
    pContext->PSSetSamplers(0, 1, pSamplerState.GetAddressOf());
    pContext->VSSetShader(ptrVertexShader ? pVertexShaders[ptrVertexShader->GetID()].Get() : pDefaultVertexShader.Get(),
                          nullptr, 0);
    pContext->PSSetShader(ptrPixelShader ? pPixelShaders[ptrPixelShader->GetID()].Get() : pDefaultPixelShader.Get(),
                          nullptr, 0);
    pContext->VSSetConstantBuffers(0, 1, pMatrixBuffer.GetAddressOf());
    pContext->DrawIndexed(renderInfo.numOfVertexIndices, 0, 0);
}

bool DirectXRenderer::SwitchToFullScreen()
{
    HRESULT hr = pSwapChain->SetFullscreenState(TRUE, NULL);
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: failed to switch to the full screen mode.");
        return false;
    }
    ReleaseBackBuffer();
    hr = pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: failed to resize the back buffer.");
        return false;
    }
    return SetupBackBuffer();
}

bool DirectXRenderer::SwitchToWindowMode()
{
    HRESULT hr = pSwapChain->SetFullscreenState(FALSE, NULL);
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: failed to switch to the full screen mode.");
        return false;
    }
    ReleaseBackBuffer();
    hr = pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    if (FAILED(hr))
    {
        PRINTLN_ERR("DirectXRenderer: failed to resize the back buffer.");
        return false;
    }
    return SetupBackBuffer();
}

DirectXRenderer::VertexInfo *DirectXRenderer::CreateInputBuffer(const IRenderable &renderable,
                                                                Scene::SceneType sceneType)
{
    auto renderInfo = renderable.GetRenderInfo();
    const auto vertexBuffer = renderInfo.vertexBuffer;
    const auto vertexBufferSize = renderInfo.numOfVertices;
    const auto colorBuffer = renderInfo.colorBuffer;
    const auto colorIndexBuffer = renderInfo.colorIndexBuffer;
    const auto colorIndexBufferSize = renderInfo.numOfColorIndices;
    const auto texcoordBuffer = renderInfo.textureCoordinates;
    const auto numOfTexcoords = renderInfo.numOfTextureCoordinates;
    const auto pTexture = renderInfo.pTexture;
    if (!vertexBuffer || !vertexBufferSize)
        return nullptr;
    VertexInfo *inputBuffer = new VertexInfo[vertexBufferSize];
    for (size_t i = 0; i < vertexBufferSize; i++)
    {
        inputBuffer[i].position = DirectX::XMFLOAT3(vertexBuffer[i].x, vertexBuffer[i].y, -vertexBuffer[i].z);
        if (i < colorIndexBufferSize)
            inputBuffer[i].color =
                DirectX::XMFLOAT4(colorBuffer[colorIndexBuffer[i]].R, colorBuffer[colorIndexBuffer[i]].G,
                                  colorBuffer[colorIndexBuffer[i]].B, colorBuffer[colorIndexBuffer[i]].A);
        else
            inputBuffer[i].color = DirectX::XMFLOAT4(0, 0, 0, 1);
        if (pTexture && i < numOfTexcoords)
        {
            inputBuffer[i].texCoord = DirectX::XMFLOAT2(texcoordBuffer[i].x, texcoordBuffer[i].y);
            inputBuffer[i].cmode = 1;
        }
        else
        {
            inputBuffer[i].texCoord = DirectX::XMFLOAT2(0, 0);
            inputBuffer[i].cmode = 0;
        }

        inputBuffer[i].gui = sceneType == Scene::SceneType::Game ? 0 : 1;
    }
    return inputBuffer;
}

bool DirectXRenderer::LoadScene(Scene &scene)
{
    auto &renderables = scene.GetRenderables();
    for (size_t i = 0; i < renderables.Length(); i++)
    {
        if (!LoadRenderable(*renderables[i], scene.GetType()))
            return false;
    }
    if (const auto pCamera = scene.GetCamera())
        ApplyCamera(pCamera);
    return true;
}

bool DirectXRenderer::LoadGUILayout(GUILayout &layout)
{
    auto &pLayers = layout.GetLayers();
    for (size_t i = 0; i < pLayers.Length(); i++)
    {
        if (!LoadRenderable(*pLayers[i], Scene::SceneType::GUI))
            return false;
        auto &pGUIs = pLayers[i]->GetComponents();
        for (size_t j = 0; j < pGUIs.Length(); j++)
        {
            if (!LoadRenderable(*pGUIs[j], Scene::SceneType::GUI))
                return false;
        }
    }
    return true;
}

bool DirectXRenderer::AddVertexShader(Shader &shader)
{
    if (shader.IsLoaded())
        return true;
    const String &path = shader.GetPath();
    ComPtr<ID3D11VertexShader> pVertexShader;
    if (!LoadVertexShaderFromFile(path, pVertexShader))
    {
        PRINTLN_ERR("DirectXRenderer: failed to compile the vertex shader.");
        return false;
    }
    shader.identifier = pVertexShaders.Length();
    pVertexShaders.Append(pVertexShader);
    return true;
}

bool DirectXRenderer::AddPixelShader(Shader &shader)
{
    if (shader.IsLoaded())
        return true;
    auto path = shader.GetPath();
    ComPtr<ID3D11PixelShader> pPixelShader;
    if (!LoadPixelShaderFromFile(path, pPixelShader))
    {
        PRINTLN_ERR("DirectXRenderer: failed to compile the pixel shader.");
        return false;
    }
    shader.identifier = pPixelShaders.Length();
    pPixelShaders.Append(pPixelShader);
    return true;
}

void DirectXRenderer::ApplyCamera(WeakPtr<Camera> pCamera)
{
    this->pCamera = pCamera;
    UpdateConstantBuffer();
}

void DirectXRenderer::OnCameraChanged()
{
    const auto pos = pCamera->GetPosition();
    const auto focalPointPos = pCamera->GetFocalPointPosition();
    const auto upVec = pCamera->GetUpVector();
    DirectX::XMStoreFloat4x4(&matrixBuffer.view,
                             DirectX::XMMatrixTranspose(DirectX::XMMatrixLookAtLH(
                                 DirectX::XMVectorSet(pos.x, pos.y, -pos.z, 0.f),
                                 DirectX::XMVectorSet(focalPointPos.x, focalPointPos.y, -focalPointPos.z, 0.f),
                                 DirectX::XMVectorSet(upVec.x, upVec.y, -upVec.z, 0.f))));
    const float aspectRatioX = static_cast<float>(backBufferDesc.Width) / backBufferDesc.Height;
    const float aspectRatioY = aspectRatioX < (16.0f / 9.0f) ? aspectRatioX / (16.0f / 9.0f) : 1.0f;
    DirectX::XMStoreFloat4x4(
        &matrixBuffer.projection,
        DirectX::XMMatrixTranspose(DirectX::XMMatrixPerspectiveFovLH(
            2.f * std::atan(std::tan(DirectX::XMConvertToRadians(pCamera->GetAngleOfView()) * 0.5f) / aspectRatioY),
            aspectRatioX, pCamera->GetDistanceToNearPlane(), pCamera->GetDistanceToFarPlane())));
}

void DirectXRenderer::OnWindowResized(long newWidth, long newHeight)
{
    ReleaseBackBuffer();
    pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
    SetupBackBuffer();
}

void DirectXRenderer::Update()
{
}

void DirectXRenderer::Render(Scene &scene)
{
    const FLOAT backgroundColor[] = {0.f, 0.f, 0.f, 1.f};
    pContext->ClearRenderTargetView(pRenderTarget.Get(), backgroundColor);
    pContext->ClearDepthStencilView(pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    pContext->OMSetRenderTargets(1, pRenderTarget.GetAddressOf(), pDepthStencilView.Get());

    UINT stride = sizeof(VertexInfo);
    UINT offset = 0;
    auto &renderables = scene.GetRenderables();
    for (size_t i = 0; i < renderables.Length(); i++)
        RenderRenderable(*renderables[i]);
    pSwapChain->Present(1, 0);
}

void DirectXRenderer::Render(GUILayout &layout)
{
    const FLOAT backgroundColor[] = {0.f, 0.f, 0.f, 1.f};
    pContext->ClearRenderTargetView(pRenderTarget.Get(), backgroundColor);
    pContext->ClearDepthStencilView(pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    pContext->OMSetRenderTargets(1, pRenderTarget.GetAddressOf(), pDepthStencilView.Get());

    auto &pLayers = layout.GetLayers();
    for (size_t i = 0; i < pLayers.Length(); i++)
    {
        RenderRenderable(*pLayers[i]);
        auto &pGUIs = pLayers[i]->GetComponents();
        for (size_t j = 0; j < pGUIs.Length(); j++)
            RenderRenderable(*pGUIs[j]);
    }
    pSwapChain->Present(1, 0);
}

void DirectXRenderer::Clear()
{
    const FLOAT backgroundColor[] = {0.f, 0.f, 0.f, 1.f};
    pContext->ClearRenderTargetView(pRenderTarget.Get(), backgroundColor);
    pContext->ClearDepthStencilView(pDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
    pContext->OMSetRenderTargets(1, pRenderTarget.GetAddressOf(), pDepthStencilView.Get());
    pSwapChain->Present(1, 0);
}
