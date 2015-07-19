#pragma once

#include <TestFramework/Framework/TestBaseClass.h>
#include <RendererFoundation/Device/Device.h>
#include <System/Window/Window.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <CoreUtils/Geometry/GeomUtils.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/RenderContext/RenderContext.h>

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
  virtual void SetupSubTests() override { }
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier) override { return ezTestAppRun::Quit; }

  virtual ezResult InitializeTest() override { return EZ_SUCCESS; }
  virtual ezResult DeInitializeTest() override { return EZ_SUCCESS; }
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;

  ezSizeU32 GetResolution() const;

protected:
  ezResult SetupRenderer(ezUInt32 uiResolutionX = 960, ezUInt32 uiResolutionY = 540);
  void ShutdownRenderer();
  void ClearScreen(const ezColor& color = ezColor::Black);

  void BeginFrame();
  void EndFrame();

  ezMeshBufferResourceHandle CreateMesh(const ezGeometry& geom, const char* szResourceName);
  ezMeshBufferResourceHandle CreateSphere(ezInt32 iSubDivs, float fRadius);
  ezMeshBufferResourceHandle CreateTorus(ezInt32 iSubDivs, float fInnerRadius, float fOuterRadius);
  ezMeshBufferResourceHandle CreateBox(float fWidth, float fHeight, float fDepth);
  void RenderObject(ezMeshBufferResourceHandle hObject, const ezMat4& mTransform, const ezColor& color, ezBitflags<ezShaderBindFlags> ShaderBindFlags = ezShaderBindFlags::Default);

  ezWindow* m_pWindow;
  ezGALDevice* m_pDevice;

  ezConstantBufferResourceHandle m_hObjectTransformCB;
  ezShaderResourceHandle m_hShader;
};


