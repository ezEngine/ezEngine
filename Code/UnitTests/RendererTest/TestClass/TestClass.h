#pragma once

#include <Core/Graphics/Geometry.h>
#include <Core/System/Window.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererFoundation/Device/Device.h>
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
  ezGraphicsTest();

  virtual ezResult GetImage(ezImage& img) override;

protected:
  virtual void SetupSubTests() override {}
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override { return ezTestAppRun::Quit; }

  virtual ezResult InitializeTest() override { return EZ_SUCCESS; }
  virtual ezResult DeInitializeTest() override { return EZ_SUCCESS; }
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;

  ezSizeU32 GetResolution() const;

protected:
  ezResult SetupRenderer();
  ezResult CreateWindow(ezUInt32 uiResolutionX = 960, ezUInt32 uiResolutionY = 540);

  void ShutdownRenderer();
  void DestroyWindow();
  void ClearScreen(const ezColor& color = ezColor::Black);
  void SetClipSpace();

  void BeginFrame();
  void EndFrame();

  ezMeshBufferResourceHandle CreateMesh(const ezGeometry& geom, const char* szResourceName);
  ezMeshBufferResourceHandle CreateSphere(ezInt32 iSubDivs, float fRadius);
  ezMeshBufferResourceHandle CreateTorus(ezInt32 iSubDivs, float fInnerRadius, float fOuterRadius);
  ezMeshBufferResourceHandle CreateBox(float fWidth, float fHeight, float fDepth);
  ezMeshBufferResourceHandle CreateLineBox(float fWidth, float fHeight, float fDepth);
  void RenderObject(ezMeshBufferResourceHandle hObject, const ezMat4& mTransform, const ezColor& color, ezBitflags<ezShaderBindFlags> ShaderBindFlags = ezShaderBindFlags::Default);

  ezWindow* m_pWindow = nullptr;
  ezGALDevice* m_pDevice = nullptr;
  ezGALSwapChainHandle m_hSwapChain;
  ezGALPass* m_pPass = nullptr;

  ezConstantBufferStorageHandle m_hObjectTransformCB;
  ezShaderResourceHandle m_hShader;
  ezGALTextureHandle m_hDepthStencilTexture;
};
