#ifndef DIRECTX_RENDERER_HPP
#define DIRECTX_RENDERER_HPP

#include "Data/DynamicArray.hpp"
#include "DirectXHelper.hpp"
#include "Display/IRenderer.hpp"
#include "Display/Scene.hpp"
#include "System/Debug/Debug.hpp"

using namespace Microsoft::WRL;

class DirectXRenderer : public IRenderer
{
  private:
    typedef struct
    {
        DirectX::XMFLOAT4X4 world;
        DirectX::XMFLOAT4X4 view;
        DirectX::XMFLOAT4X4 projection;
    } MatrixBuffer;

    typedef struct
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
        DirectX::XMFLOAT2 texCoord;
        UINT cmode;
        UINT gui;
    } VertexInfo;

    MatrixBuffer matrixBuffer;

    ComPtr<ID3D11Device> pDevice;
    ComPtr<ID3D11RasterizerState> pRasterizerState;
    ComPtr<ID3D11DeviceContext> pContext;
    ComPtr<IDXGISwapChain> pSwapChain;
    ComPtr<ID3D11Texture2D> pBackBuffer;
    D3D11_TEXTURE2D_DESC backBufferDesc;
    ComPtr<ID3D11RenderTargetView> pRenderTarget;
    ComPtr<ID3D11Texture2D> pDepthStencil;
    ComPtr<ID3D11DepthStencilView> pDepthStencilView;
    D3D11_VIEWPORT viewport;
    D3D_FEATURE_LEVEL featureLevel;
    ComPtr<ID3D11VertexShader> pDefaultVertexShader;
    ComPtr<ID3D11PixelShader> pDefaultPixelShader;
    ComPtr<ID3D11InputLayout> pInputLayout;
    ComPtr<ID3D11Buffer> pMatrixBuffer;
    DynamicArray<ComPtr<ID3D11VertexShader>> pVertexShaders;
    DynamicArray<ComPtr<ID3D11PixelShader>> pPixelShaders;
    DynamicArray<ComPtr<ID3D11Buffer>> pVertexBuffers;
    DynamicArray<ComPtr<ID3D11Buffer>> pIndexBuffers;
    DynamicArray<ComPtr<ID3D11Texture2D>> pTextures;
    DynamicArray<ComPtr<ID3D11ShaderResourceView>> pShaderResourceViews;
    ComPtr<ID3D11SamplerState> pSamplerState;

    const Camera *pCamera;

    void UpdateConstantBuffer();

    bool SetupBackBuffer();

    void ReleaseBackBuffer();

    bool LoadVertexShaderFromFile(const String &path, ComPtr<ID3D11VertexShader> &pVertexShader);

    bool LoadPixelShaderFromFile(const String &path, ComPtr<ID3D11PixelShader> &pPixelShader);

    bool LoadDefaultVertexShader();

    bool LoadDefaultPixelShader();

    bool CompileDefaultShaders();

    bool InitializeTextureSampleState();

    bool LoadTexture(Texture *pTexture);

    bool LoadRenderable(IRenderable &renderable, Scene::SceneType sceneType);

    static const char DefaultVertexShader[];

    static const char DefaultPixelShader[];

    static const D3D11_INPUT_ELEMENT_DESC InputDescs[];

    static VertexInfo *CreateInputBuffer(const IRenderable &renderable, Scene::SceneType sceneType);

  public:
    DirectXRenderer();

    ~DirectXRenderer();

    virtual bool Initialize(HWND windowHandle, bool fullScreen) override;

    virtual bool SwitchToFullScreen() override;

    virtual bool SwitchToWindowMode() override;

    virtual bool LoadScene(Scene &scene) override;

    virtual bool AddVertexShader(Shader *pShader) override;

    virtual bool AddPixelShader(Shader *pShader) override;

    virtual void ApplyCamera(const Camera *pCamera) override;

    virtual void OnCameraChanged() override;

    virtual void OnWindowResized(long newWidth, long newHeight) override;

    virtual void Update() override;

    virtual void Render(const Scene &scene) override;

    virtual void Clear() override;
};

#endif
