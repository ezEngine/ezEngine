#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Textures/Texture2DResource.h>

class ezRendererTestPipelineStates : public ezGraphicsTest
{
public:
  virtual const char* GetTestName() const override { return "PipelineStates"; }

private:
  enum SubTests
  {
    ST_MostBasicShader,
    ST_ViewportScissor,
    ST_VertexBuffer,
    ST_IndexBuffer,
    ST_ConstantBuffer,
    ST_StructuredBuffer,
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("01 - MostBasicShader", SubTests::ST_MostBasicShader);
    AddSubTest("02 - ViewportScissor", SubTests::ST_ViewportScissor);
    AddSubTest("03 - VertexBuffer", SubTests::ST_VertexBuffer);
    AddSubTest("04 - IndexBuffer", SubTests::ST_IndexBuffer);
    AddSubTest("05 - ConstantBuffer", SubTests::ST_ConstantBuffer);
    AddSubTest("06 - StructuredBuffer", SubTests::ST_StructuredBuffer);
  }

  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  void RenderBlock(ezMeshBufferResourceHandle mesh, ezColor clearColor = ezColor::CornflowerBlue, ezUInt32 uiRenderTargetClearMask = 0xFFFFFFFF, ezRectFloat* pViewport = nullptr, ezRectU32* pScissor = nullptr);

  ezGALRenderCommandEncoder* BeginRendering(ezColor clearColor, ezUInt32 uiRenderTargetClearMask, ezRectFloat* pViewport = nullptr, ezRectU32* pScissor = nullptr);
  void EndRendering();

  void MostBasicTriangleTest();
  void ViewportScissorTest();
  void VertexBufferTest();
  void IndexBufferTest();
  void ConstantBufferTest();
  void StructuredBufferTest();

private:
  ezShaderResourceHandle m_hMostBasicTriangleShader;
  ezShaderResourceHandle m_hNDCPositionOnlyShader;
  ezShaderResourceHandle m_hConstantBufferShader;
  ezShaderResourceHandle m_hInstancingShader;

  ezMeshBufferResourceHandle m_hTriangleMesh;
  ezMeshBufferResourceHandle m_hSphereMesh;

  ezConstantBufferStorageHandle m_hTestColorsConstantBuffer;
  ezConstantBufferStorageHandle m_hTestPositionsConstantBuffer;

  ezGALBufferHandle m_hInstancingData;
};
