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
    ST_Texture2D,
    ST_Texture2DArray,
    ST_GenerateMipMaps,
    ST_PushConstants,
    ST_SetsSlots,
    ST_Timestamps,
  };

  enum ImageCaptureFrames
  {
    DefaultCapture = 5,
    StructuredBuffer_InitialData = 5,
    StructuredBuffer_Discard = 6,
    StructuredBuffer_NoOverwrite = 8,
    StructuredBuffer_CopyToTempStorage = 9,
    Timestamps_MaxWaitTime = ezMath::MaxValue<ezUInt32>(),
  };

  virtual void SetupSubTests() override
  {
    AddSubTest("01 - MostBasicShader", SubTests::ST_MostBasicShader);
    AddSubTest("02 - ViewportScissor", SubTests::ST_ViewportScissor);
    AddSubTest("03 - VertexBuffer", SubTests::ST_VertexBuffer);
    AddSubTest("04 - IndexBuffer", SubTests::ST_IndexBuffer);
    AddSubTest("05 - ConstantBuffer", SubTests::ST_ConstantBuffer);
    AddSubTest("06 - StructuredBuffer", SubTests::ST_StructuredBuffer);
    AddSubTest("07 - Texture2D", SubTests::ST_Texture2D);
    AddSubTest("08 - Texture2DArray", SubTests::ST_Texture2DArray);
    AddSubTest("09 - GenerateMipMaps", SubTests::ST_GenerateMipMaps);
    AddSubTest("10 - PushConstants", SubTests::ST_PushConstants);
    AddSubTest("11 - SetsSlots", SubTests::ST_SetsSlots);
    //AddSubTest("12 - Timestamps", SubTests::ST_Timestamps); // Disabled due to CI failure on AMD.
  }

  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

  void RenderBlock(ezMeshBufferResourceHandle mesh, ezColor clearColor = ezColor::CornflowerBlue, ezUInt32 uiRenderTargetClearMask = 0xFFFFFFFF, ezRectFloat* pViewport = nullptr, ezRectU32* pScissor = nullptr);

  void MostBasicTriangleTest();
  void ViewportScissorTest();
  void VertexBufferTest();
  void IndexBufferTest();
  void ConstantBufferTest();
  void StructuredBufferTest();
  void Texture2D();
  void Texture2DArray();
  void GenerateMipMaps();
  void PushConstantsTest();
  void SetsSlotsTest();
  void Timestamps();

private:

  ezShaderResourceHandle m_hMostBasicTriangleShader;
  ezShaderResourceHandle m_hNDCPositionOnlyShader;
  ezShaderResourceHandle m_hConstantBufferShader;
  ezShaderResourceHandle m_hPushConstantsShader;
  ezShaderResourceHandle m_hInstancingShader;

  ezMeshBufferResourceHandle m_hTriangleMesh;
  ezMeshBufferResourceHandle m_hSphereMesh;

  ezConstantBufferStorageHandle m_hTestPerFrameConstantBuffer;
  ezConstantBufferStorageHandle m_hTestColorsConstantBuffer;
  ezConstantBufferStorageHandle m_hTestPositionsConstantBuffer;

  ezGALBufferHandle m_hInstancingData;
  ezGALResourceViewHandle m_hInstancingDataView_8_4;
  ezGALResourceViewHandle m_hInstancingDataView_12_4;

  ezGALTextureHandle m_hTexture2D;
  ezGALResourceViewHandle m_hTexture2D_Mip0;
  ezGALResourceViewHandle m_hTexture2D_Mip1;
  ezGALResourceViewHandle m_hTexture2D_Mip2;
  ezGALResourceViewHandle m_hTexture2D_Mip3;
  ezGALTextureHandle m_hTexture2DArray;
  ezGALResourceViewHandle m_hTexture2DArray_Layer0_Mip0;
  ezGALResourceViewHandle m_hTexture2DArray_Layer0_Mip1;
  ezGALResourceViewHandle m_hTexture2DArray_Layer1_Mip0;
  ezGALResourceViewHandle m_hTexture2DArray_Layer1_Mip1;

  bool m_bTimestampsValid = false;
  ezTime m_CPUTime[2];
  ezTime m_GPUTime[2];
  ezGALTimestampHandle m_timestamps[2];
};
