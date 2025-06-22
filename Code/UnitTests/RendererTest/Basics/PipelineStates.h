#pragma once

#include "../TestClass/TestClass.h"
#include <RendererCore/Meshes/MeshBufferResource.h>
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
    ST_TexelBuffer,
    ST_ByteAddressBuffer,
    ST_Texture2D,
    ST_Texture2DArray,
    ST_GenerateMipMaps,
    ST_PushConstants,
    ST_SetsSlots,
    ST_Timestamps,
    ST_OcclusionQueries,
    ST_CustomVertexStreams,
  };

  enum ImageCaptureFrames
  {
    DefaultCapture = 5,
    StructuredBuffer_InitialData = 5,
    StructuredBuffer_UpdateForNextFrame = 6,
    StructuredBuffer_UpdateForNextFrame2 = 7,
    StructuredBuffer_Transient1 = 8,
    StructuredBuffer_Transient2 = 9,
    StructuredBuffer_UAV = 10,
    CustomVertexStreams_Offsets = 6,
    Timestamps_MaxWaitTime = ezMath::MaxValue<ezUInt32>(),
  };

  virtual void SetupSubTests() override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;
  virtual void MapImageNumberToString(const char* szTestName, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber, ezStringBuilder& out_sString) const override;

  void RenderBlock(ezMeshBufferResourceHandle mesh, ezColor clearColor = ezColor::CornflowerBlue, ezUInt32 uiRenderTargetClearMask = 0xFFFFFFFF, ezRectFloat* pViewport = nullptr, ezRectU32* pScissor = nullptr);

  void MostBasicTriangleTest();
  void ViewportScissorTest();
  void VertexBufferTest();
  void IndexBufferTest();
  void ConstantBufferTest();
  void StructuredBufferTestUpload();
  void StructuredBufferTest(ezGALShaderResourceType::Enum bufferType);
  void Texture2D();
  void Texture2DArray();
  void GenerateMipMaps();
  void PushConstantsTest();
  void SetsSlotsTest();
  void CustomVertexStreams();
  ezTestAppRun Timestamps();
  ezTestAppRun OcclusionQueries();

private:
  ezShaderResourceHandle m_hMostBasicTriangleShader;
  ezShaderResourceHandle m_hNDCPositionOnlyShader;
  ezShaderResourceHandle m_hConstantBufferShader;
  ezShaderResourceHandle m_hPushConstantsShader;
  ezShaderResourceHandle m_hInstancingShader;
  ezShaderResourceHandle m_hCopyBufferShader;
  ezShaderResourceHandle m_hCustomVertexStreamShader;

  ezMeshBufferResourceHandle m_hTriangleMesh;
  ezMeshBufferResourceHandle m_hSphereMesh;

  ezConstantBufferStorageHandle m_hTestPerFrameConstantBuffer;
  ezConstantBufferStorageHandle m_hTestColorsConstantBuffer;
  ezConstantBufferStorageHandle m_hTestPositionsConstantBuffer;

  ezGALBufferHandle m_hInstancingData;
  ezGALBufferHandle m_hInstancingDataTransient;
  ezGALBufferHandle m_hInstancingDataUAV;

  ezGALBufferHandle m_hInstancingDataCustomVertexStream;
  ezHybridArray<ezVertexStreamInfo, 4> m_CustomVertexStreams;

  ezGALTextureHandle m_hTexture2D;
  ezGALTextureHandle m_hTexture2DArray;


  // Timestamps / Occlusion Queries test
  ezInt32 m_iDelay = 0;
  ezTime m_CPUTime[2];
  ezTime m_GPUTime[2];
  ezGALTimestampHandle m_timestamps[2];
  ezGALOcclusionHandle m_queries[4];
  ezGALFenceHandle m_hFence = {};
};
