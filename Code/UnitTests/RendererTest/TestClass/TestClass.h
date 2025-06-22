#pragma once

#include <Core/Graphics/Geometry.h>
#include <Core/System/Window.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/ReadbackHelper.h>
#include <TestFramework/Framework/TestBaseClass.h>

#undef CreateWindow

class ezImage;

struct ObjectCB
{
  ezMat4 m_MVP;
  ezColor m_Color;
};

class ezGraphicsTest : public ezTestBaseClass
{
public:
  static ezResult CreateRenderer(ezGALDevice*& out_pDevice);
  static void SetClipSpace();

public:
  ezGraphicsTest();

  virtual ezResult GetImage(ezImage& ref_img, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber) override;

protected:
  virtual void SetupSubTests() override {}
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override { return ezTestAppRun::Quit; }

  virtual ezResult InitializeTest() override;
  virtual ezResult DeInitializeTest() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;

  ezSizeU32 GetResolution() const;

protected:
  const ezGALDeviceCapabilities& GetDeviceCapabilities();
  ezResult SetupRenderer();
  void ShutdownRenderer();

  ezResult CreateWindow(ezUInt32 uiResolutionX = 960, ezUInt32 uiResolutionY = 540);
  void DestroyWindow();

  void BeginFrame();
  void EndFrame();

  void BeginCommands(const char* szPassName);
  void EndCommands();

  ezGALCommandEncoder* BeginRendering(ezColor clearColor, ezUInt32 uiRenderTargetClearMask = 0xFFFFFFFF, ezRectFloat* pViewport = nullptr, ezRectU32* pScissor = nullptr);
  void EndRendering();

  /// \brief Renders a unit cube and makes an image comparison if m_bCaptureImage is set and the current frame is in m_ImgCompFrames.
  /// \param viewport Viewport to render into.
  /// \param mMVP Model View Projection matrix for camera. Use CreateSimpleMVP for convenience.
  /// \param uiRenderTargetClearMask What render targets if any should be cleared.
  /// \param hTexture The texture to render onto the cube.
  /// \param textureRange The texture range to use when rendering the cube.
  void RenderCube(ezRectFloat viewport, ezMat4 mMVP, ezUInt32 uiRenderTargetClearMask, ezGALTextureHandle hTexture, const ezGALTextureRange& textureRange = {});

  ezMat4 CreateSimpleMVP(float fAspectRatio);


  ezMeshBufferResourceHandle CreateMesh(const ezGeometry& geom, const char* szResourceName);
  ezMeshBufferResourceHandle CreateSphere(ezInt32 iSubDivs, float fRadius);
  ezMeshBufferResourceHandle CreateTorus(ezInt32 iSubDivs, float fInnerRadius, float fOuterRadius);
  ezMeshBufferResourceHandle CreateBox(float fWidth, float fHeight, float fDepth);
  ezMeshBufferResourceHandle CreateLineBox(float fWidth, float fHeight, float fDepth);
  void RenderObject(ezMeshBufferResourceHandle hObject, const ezMat4& mTransform, const ezColor& color, ezBitflags<ezShaderBindFlags> ShaderBindFlags = ezShaderBindFlags::Default);

  ezWindow* m_pWindow = nullptr;
  ezGALDevice* m_pDevice = nullptr;
  ezGALCommandEncoder* m_pEncoder = nullptr;

  ezGALSwapChainHandle m_hSwapChain;
  ezGALTextureHandle m_hDepthStencilTexture;

  ezConstantBufferStorageHandle m_hObjectTransformCB;
  ezShaderResourceHandle m_hShader;
  ezMeshBufferResourceHandle m_hCubeUV;

  ezInt32 m_iFrame = 0;
  bool m_bCaptureImage = false;
  ezHybridArray<ezUInt32, 8> m_ImgCompFrames;
  ezGALReadbackTextureHelper m_Readback;
};
