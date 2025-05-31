#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <RendererTest/Basics/PipelineStates.h>
#include <RendererTest/Basics/RendererTestUtils.h>

#include <RendererTest/../../../Data/UnitTests/RendererTest/Shaders/TestConstants.h>
#include <RendererTest/../../../Data/UnitTests/RendererTest/Shaders/TestPushConstants.h>


void ezRendererTestPipelineStates::SetupSubTests()
{
  const ezGALDeviceCapabilities& caps = GetDeviceCapabilities();

  AddSubTest("01 - MostBasicShader", SubTests::ST_MostBasicShader);
  AddSubTest("02 - ViewportScissor", SubTests::ST_ViewportScissor);
  AddSubTest("03 - VertexBuffer", SubTests::ST_VertexBuffer);
  AddSubTest("04 - IndexBuffer", SubTests::ST_IndexBuffer);
  AddSubTest("05 - ConstantBuffer", SubTests::ST_ConstantBuffer);
  AddSubTest("06 - StructuredBuffer", SubTests::ST_StructuredBuffer);
  if (caps.m_bSupportsTexelBuffer)
  {
    AddSubTest("06b - TexelBuffer", SubTests::ST_TexelBuffer);
  }
  AddSubTest("06c - ByteAddressBuffer", SubTests::ST_ByteAddressBuffer);
  AddSubTest("07 - Texture2D", SubTests::ST_Texture2D);
  AddSubTest("08 - Texture2DArray", SubTests::ST_Texture2DArray);
  AddSubTest("09 - GenerateMipMaps", SubTests::ST_GenerateMipMaps);
  AddSubTest("10 - PushConstants", SubTests::ST_PushConstants);
  AddSubTest("11 - SetsSlots", SubTests::ST_SetsSlots);
  AddSubTest("12 - Timestamps", SubTests::ST_Timestamps); // Disabled due to CI failure on AMD.
  AddSubTest("13 - OcclusionQueries", SubTests::ST_OcclusionQueries);
  AddSubTest("14 - CustomVertexStreams", SubTests::ST_CustomVertexStreams);
}

ezResult ezRendererTestPipelineStates::InitializeSubTest(ezInt32 iIdentifier)
{
  {
    m_iDelay = 0;
    m_CPUTime[0] = {};
    m_CPUTime[1] = {};
    m_GPUTime[0] = {};
    m_GPUTime[1] = {};
    m_timestamps[0] = {};
    m_timestamps[1] = {};
    m_queries[0] = {};
    m_queries[1] = {};
    m_queries[2] = {};
    m_queries[3] = {};
    m_hFence = {};
  }

  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));
  EZ_SUCCEED_OR_RETURN(CreateWindow(320, 240));
  m_hMostBasicTriangleShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/MostBasicTriangle.ezShader");
  m_hNDCPositionOnlyShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/NDCPositionOnly.ezShader");
  m_hConstantBufferShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/ConstantBuffer.ezShader");
  m_hPushConstantsShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/PushConstants.ezShader");
  m_hCustomVertexStreamShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/CustomVertexStreams.ezShader");

  {
    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AllocateStreams(3);

    if (ezClipSpaceYMode::RenderToTextureDefault == ezClipSpaceYMode::Flipped)
    {
      desc.SetVertexData<ezVec3>(0, 0, ezVec3(1.f, 1.f, 0.0f));
      desc.SetVertexData<ezVec3>(0, 1, ezVec3(-1.f, 1.f, 0.0f));
      desc.SetVertexData<ezVec3>(0, 2, ezVec3(0.f, -1.f, 0.0f));
    }
    else
    {
      desc.SetVertexData<ezVec3>(0, 0, ezVec3(1.f, -1.f, 0.0f));
      desc.SetVertexData<ezVec3>(0, 1, ezVec3(-1.f, -1.f, 0.0f));
      desc.SetVertexData<ezVec3>(0, 2, ezVec3(0.f, 1.f, 0.0f));
    }

    m_hTriangleMesh = ezResourceManager::CreateResource<ezMeshBufferResource>("UnitTest-TriangleMesh", std::move(desc), "TriangleMesh");
  }
  {
    ezGeometry geom;
    geom.AddStackedSphere(0.5f, 16, 16);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

    m_hSphereMesh = ezResourceManager::CreateResource<ezMeshBufferResource>("UnitTest-SphereMesh", std::move(desc), "SphereMesh");
  }
  m_hTestPerFrameConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezTestPerFrame>();
  m_hTestColorsConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezTestColors>();
  m_hTestPositionsConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezTestPositions>();

  if (iIdentifier == SubTests::ST_StructuredBuffer || iIdentifier == SubTests::ST_TexelBuffer || iIdentifier == SubTests::ST_ByteAddressBuffer)
  {
    ezGALBufferCreationDescription desc;
    ezGALShaderResourceType::Enum slotType;
    desc.m_uiTotalSize = 16 * sizeof(ezTestShaderData);
    desc.m_ResourceAccess.m_bImmutable = false;
    switch (iIdentifier)
    {
      case SubTests::ST_StructuredBuffer:
        desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource;
        desc.m_uiStructSize = sizeof(ezTestShaderData);
        m_hInstancingShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/InstancingStructuredBuffer.ezShader");
        m_hCopyBufferShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/CopyStructuredBuffer.ezShader");
        slotType = ezGALShaderResourceType::StructuredBuffer;
        break;
      case SubTests::ST_TexelBuffer:
        desc.m_Format = ezGALResourceFormat::RGBAFloat;
        desc.m_BufferFlags = ezGALBufferUsageFlags::TexelBuffer | ezGALBufferUsageFlags::ShaderResource;
        m_hInstancingShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/InstancingTexelBuffer.ezShader");
        m_hCopyBufferShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/CopyTexelBuffer.ezShader");
        slotType = ezGALShaderResourceType::TexelBuffer;
        break;
      default:
      case SubTests::ST_ByteAddressBuffer:
        desc.m_BufferFlags = ezGALBufferUsageFlags::ByteAddressBuffer | ezGALBufferUsageFlags::ShaderResource;
        m_hInstancingShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/InstancingByteAddressBuffer.ezShader");
        m_hCopyBufferShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/CopyByteAddressBuffer.ezShader");
        slotType = ezGALShaderResourceType::ByteAddressBuffer;
        break;
    }

    // We only fill the first 8 elements with data. The rest is dynamically updated during testing.
    ezHybridArray<ezTestShaderData, 16> instanceData;
    ezRendererTestUtils::FillStructuredBuffer(instanceData);
    m_hInstancingData = m_pDevice->CreateBuffer(desc, instanceData.GetByteArrayPtr());

    // Create another transient variant of the buffer. If supported, this will extend support to all SRV types.
    ezGALBufferCreationDescription transientDesc = desc;
    transientDesc.m_BufferFlags |= ezGALBufferUsageFlags::Transient;
    if (m_pDevice->GetCapabilities().m_bSupportsMultipleSRVTypes)
    {
      transientDesc.m_BufferFlags |= ezGALBufferUsageFlags::StructuredBuffer;
      transientDesc.m_uiStructSize = sizeof(ezTestShaderData);
      transientDesc.m_BufferFlags |= ezGALBufferUsageFlags::ByteAddressBuffer;
      if (m_pDevice->GetCapabilities().m_bSupportsTexelBuffer)
      {
        transientDesc.m_BufferFlags |= ezGALBufferUsageFlags::TexelBuffer;
        transientDesc.m_Format = ezGALResourceFormat::RGBAFloat;
      }
    }
    m_hInstancingDataTransient = m_pDevice->CreateBuffer(transientDesc);

    // Views into main buffer
    ezGALBufferResourceViewCreationDescription viewDesc;
    viewDesc.m_ResourceType = slotType;
    viewDesc.m_hBuffer = m_hInstancingData;
    viewDesc.m_uiByteOffset = 8 * sizeof(ezTestShaderData);
    viewDesc.m_uiByteCount = 4 * sizeof(ezTestShaderData);
    m_hInstancingDataView_8_4 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiByteOffset = 12 * sizeof(ezTestShaderData);
    m_hInstancingDataView_12_4 = m_pDevice->CreateResourceView(viewDesc);

    // UAV variant and views
    ezGALBufferCreationDescription uavDesc = desc;
    uavDesc.m_uiTotalSize = 8 * sizeof(ezTestShaderData);
    uavDesc.m_BufferFlags |= ezGALBufferUsageFlags::UnorderedAccess;
    m_hInstancingDataUAV = m_pDevice->CreateBuffer(uavDesc);

    ezGALBufferUnorderedAccessViewCreationDescription uavViewDesc;
    uavViewDesc.m_hBuffer = m_hInstancingDataUAV;
    uavViewDesc.m_uiByteCount = 4 * sizeof(ezTestShaderData);
    uavViewDesc.m_uiByteOffset = 0;
    m_hInstancingDataUavView_0_4 = m_pDevice->CreateUnorderedAccessView(uavViewDesc);
    uavViewDesc.m_uiByteOffset = 4 * sizeof(ezTestShaderData);
    m_hInstancingDataUavView_4_4 = m_pDevice->CreateUnorderedAccessView(uavViewDesc);
  }


  if (iIdentifier == SubTests::ST_CustomVertexStreams)
  {
    // Save as ST_StructuredBuffer, but we put the data in a vertex buffer.
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezTestShaderData);
    desc.m_uiTotalSize = 16 * desc.m_uiStructSize;
    desc.m_BufferFlags = ezGALBufferUsageFlags::VertexBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;

    ezHybridArray<ezTestShaderData, 16> instanceData;
    ezRendererTestUtils::FillStructuredBuffer(instanceData);
    m_hInstancingDataCustomVertexStream = m_pDevice->CreateBuffer(desc, instanceData.GetByteArrayPtr());

    ezVertexStreamInfo& color = m_CustomVertexStreams.ExpandAndGetRef();
    color.m_Format = ezGALResourceFormat::XYZWFloat;
    color.m_uiVertexBufferSlot = 5;
    color.m_Semantic = ezGALVertexAttributeSemantic::Color4;
    color.m_uiElementSize = sizeof(ezVec4);
    color.m_uiOffset = sizeof(ezVec4) * 0;

    ezVertexStreamInfo& r0 = m_CustomVertexStreams.ExpandAndGetRef();
    r0.m_Format = ezGALResourceFormat::XYZWFloat;
    r0.m_uiVertexBufferSlot = 5;
    r0.m_Semantic = ezGALVertexAttributeSemantic::Color5;
    r0.m_uiElementSize = sizeof(ezVec4);
    r0.m_uiOffset = sizeof(ezVec4) * 1;

    ezVertexStreamInfo& r1 = m_CustomVertexStreams.ExpandAndGetRef();
    r1.m_Format = ezGALResourceFormat::XYZWFloat;
    r1.m_uiVertexBufferSlot = 5;
    r1.m_Semantic = ezGALVertexAttributeSemantic::Color6;
    r1.m_uiElementSize = sizeof(ezVec4);
    r1.m_uiOffset = sizeof(ezVec4) * 2;

    ezVertexStreamInfo& r2 = m_CustomVertexStreams.ExpandAndGetRef();
    r2.m_Format = ezGALResourceFormat::XYZWFloat;
    r2.m_uiVertexBufferSlot = 5;
    r2.m_Semantic = ezGALVertexAttributeSemantic::Color7;
    r2.m_uiElementSize = sizeof(ezVec4);
    r2.m_uiOffset = sizeof(ezVec4) * 3;
  }

  {
    // Texture2D
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = 8;
    desc.m_uiHeight = 8;
    desc.m_uiMipLevelCount = 4;
    desc.m_Format = ezGALResourceFormat::BGRAUByteNormalizedsRGB;

    ezImage coloredMips;
    ezRendererTestUtils::CreateImage(coloredMips, desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, true);

    if (iIdentifier == SubTests::ST_GenerateMipMaps)
    {
      // Clear all mips except the fist one and let them be regenerated.
      desc.m_ResourceAccess.m_bImmutable = false;
      desc.m_bAllowDynamicMipGeneration = true;
      for (ezUInt32 m = 1; m < desc.m_uiMipLevelCount; m++)
      {
        const ezUInt32 uiHeight = coloredMips.GetHeight(m);
        const ezUInt32 uiWidth = coloredMips.GetWidth(m);
        for (ezUInt32 y = 0; y < uiHeight; y++)
        {
          for (ezUInt32 x = 0; x < uiWidth; x++)
          {
            ezRendererTestUtils::ImgColor* pColor = coloredMips.GetPixelPointer<ezRendererTestUtils::ImgColor>(m, 0u, 0u, x, y);
            pColor->a = 255;
            pColor->b = 0;
            pColor->g = 0;
            pColor->r = 0;
          }
        }
      }
    }

    ezHybridArray<ezGALSystemMemoryDescription, 4> initialData;
    initialData.SetCount(desc.m_uiMipLevelCount);
    for (ezUInt32 m = 0; m < desc.m_uiMipLevelCount; m++)
    {
      ezGALSystemMemoryDescription& memoryDesc = initialData[m];
      memoryDesc.m_pData = coloredMips.GetSubImageView(m).GetByteBlobPtr();
      memoryDesc.m_uiRowPitch = static_cast<ezUInt32>(coloredMips.GetRowPitch(m));
      memoryDesc.m_uiSlicePitch = static_cast<ezUInt32>(coloredMips.GetDepthPitch(m));
    }
    m_hTexture2D = m_pDevice->CreateTexture(desc, initialData);

    ezGALTextureResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture = m_hTexture2D;
    viewDesc.m_uiMostDetailedMipLevel = 0;
    viewDesc.m_uiMipLevelsToUse = 1;
    m_hTexture2D_Mip0 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiMostDetailedMipLevel = 1;
    m_hTexture2D_Mip1 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiMostDetailedMipLevel = 2;
    m_hTexture2D_Mip2 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiMostDetailedMipLevel = 3;
    m_hTexture2D_Mip3 = m_pDevice->CreateResourceView(viewDesc);
  }

  {
    // Texture2DArray
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = 8;
    desc.m_uiHeight = 8;
    desc.m_uiMipLevelCount = 4;
    desc.m_uiArraySize = 2;
    desc.m_Type = ezGALTextureType::Texture2DArray;
    desc.m_Format = ezGALResourceFormat::BGRAUByteNormalizedsRGB;

    ezImage coloredMips[2];
    ezRendererTestUtils::CreateImage(coloredMips[0], desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, false, 0);
    ezRendererTestUtils::CreateImage(coloredMips[1], desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, false, 255);

    ezHybridArray<ezGALSystemMemoryDescription, 8> initialData;
    initialData.SetCount(desc.m_uiArraySize * desc.m_uiMipLevelCount);
    for (ezUInt32 l = 0; l < desc.m_uiArraySize; l++)
    {
      for (ezUInt32 m = 0; m < desc.m_uiMipLevelCount; m++)
      {
        ezGALSystemMemoryDescription& memoryDesc = initialData[m + l * desc.m_uiMipLevelCount];

        memoryDesc.m_pData = coloredMips[l].GetSubImageView(m).GetByteBlobPtr();
        memoryDesc.m_uiRowPitch = static_cast<ezUInt32>(coloredMips[l].GetRowPitch(m));
        memoryDesc.m_uiSlicePitch = static_cast<ezUInt32>(coloredMips[l].GetDepthPitch(m));
      }
    }
    m_hTexture2DArray = m_pDevice->CreateTexture(desc, initialData);

    ezGALTextureResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture = m_hTexture2DArray;
    viewDesc.m_uiMipLevelsToUse = 1;
    viewDesc.m_uiFirstArraySlice = 0;
    viewDesc.m_uiMostDetailedMipLevel = 0;
    m_hTexture2DArray_Layer0_Mip0 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiMostDetailedMipLevel = 1;
    m_hTexture2DArray_Layer0_Mip1 = m_pDevice->CreateResourceView(viewDesc);

    viewDesc.m_uiFirstArraySlice = 1;
    viewDesc.m_uiMostDetailedMipLevel = 0;
    m_hTexture2DArray_Layer1_Mip0 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiMostDetailedMipLevel = 1;
    m_hTexture2DArray_Layer1_Mip1 = m_pDevice->CreateResourceView(viewDesc);
  }

  switch (iIdentifier)
  {
    case SubTests::ST_MostBasicShader:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_ViewportScissor:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_VertexBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_IndexBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_ConstantBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_StructuredBuffer:
    case SubTests::ST_TexelBuffer:
    case SubTests::ST_ByteAddressBuffer:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_InitialData);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_UpdateForNextFrame);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_UpdateForNextFrame2);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_Transient1);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_Transient2);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_UAV);
      break;
    case SubTests::ST_GenerateMipMaps:
    case SubTests::ST_Texture2D:
    {
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2D.ezShader");
    }
    break;
    case SubTests::ST_Texture2DArray:
    {
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2DArray.ezShader");
    }
    break;
    case SubTests::ST_PushConstants:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_SetsSlots:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/SetsSlots.ezShader");
      break;
    case SubTests::ST_Timestamps:
    case SubTests::ST_OcclusionQueries:
      break;
    case SubTests::ST_CustomVertexStreams:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::CustomVertexStreams_Offsets);
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return EZ_SUCCESS;
}

ezResult ezRendererTestPipelineStates::DeInitializeSubTest(ezInt32 iIdentifier)
{
  m_hTriangleMesh.Invalidate();
  m_hSphereMesh.Invalidate();

  m_hMostBasicTriangleShader.Invalidate();
  m_hNDCPositionOnlyShader.Invalidate();
  m_hConstantBufferShader.Invalidate();
  m_hPushConstantsShader.Invalidate();
  m_hInstancingShader.Invalidate();
  m_hCopyBufferShader.Invalidate();
  m_hCustomVertexStreamShader.Invalidate();

  m_hTestPerFrameConstantBuffer.Invalidate();
  m_hTestColorsConstantBuffer.Invalidate();
  m_hTestPositionsConstantBuffer.Invalidate();

  if (!m_hInstancingData.IsInvalidated())
  {
    m_pDevice->DestroyBuffer(m_hInstancingData);
    m_hInstancingData.Invalidate();
  }
  if (!m_hInstancingDataTransient.IsInvalidated())
  {
    m_pDevice->DestroyBuffer(m_hInstancingDataTransient);
    m_hInstancingDataTransient.Invalidate();
  }
  if (!m_hInstancingDataUAV.IsInvalidated())
  {
    m_pDevice->DestroyBuffer(m_hInstancingDataUAV);
    m_hInstancingDataUAV.Invalidate();
  }
  m_hInstancingDataView_8_4.Invalidate();
  m_hInstancingDataView_12_4.Invalidate();
  m_hInstancingDataUavView_0_4.Invalidate();
  m_hInstancingDataUavView_4_4.Invalidate();

  if (!m_hInstancingDataCustomVertexStream.IsInvalidated())
  {
    m_pDevice->DestroyBuffer(m_hInstancingDataCustomVertexStream);
    m_hInstancingDataCustomVertexStream.Invalidate();
  }
  m_CustomVertexStreams.Clear();

  if (!m_hTexture2D.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hTexture2D);
    m_hTexture2D.Invalidate();
  }
  if (!m_hTexture2DArray.IsInvalidated())
  {
    m_pDevice->DestroyTexture(m_hTexture2DArray);
    m_hTexture2DArray.Invalidate();
  }
  m_hTexture2D_Mip0.Invalidate();
  m_hTexture2D_Mip1.Invalidate();
  m_hTexture2D_Mip2.Invalidate();
  m_hTexture2D_Mip3.Invalidate();
  m_hShader.Invalidate();

  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_queries); i++)
  {
    m_queries[i] = {};
  }
  m_hFence = {};

  DestroyWindow();
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::DeInitializeSubTest(iIdentifier));
  return EZ_SUCCESS;
}

ezTestAppRun ezRendererTestPipelineStates::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;
  m_bCaptureImage = false;

  if (iIdentifier == SubTests::ST_StructuredBuffer || iIdentifier == SubTests::ST_TexelBuffer || iIdentifier == SubTests::ST_ByteAddressBuffer)
  {
    StructuredBufferTestUpload();
  }

  BeginFrame();

  switch (iIdentifier)
  {
    case SubTests::ST_MostBasicShader:
      MostBasicTriangleTest();
      break;
    case SubTests::ST_ViewportScissor:
      ViewportScissorTest();
      break;
    case SubTests::ST_VertexBuffer:
      VertexBufferTest();
      break;
    case SubTests::ST_IndexBuffer:
      IndexBufferTest();
      break;
    case SubTests::ST_ConstantBuffer:
      ConstantBufferTest();
      break;
    case SubTests::ST_StructuredBuffer:
      StructuredBufferTest(ezGALShaderResourceType::StructuredBuffer);
      break;
    case SubTests::ST_TexelBuffer:
      StructuredBufferTest(ezGALShaderResourceType::TexelBuffer);
      break;
    case SubTests::ST_ByteAddressBuffer:
      StructuredBufferTest(ezGALShaderResourceType::ByteAddressBuffer);
      break;
    case SubTests::ST_Texture2D:
      Texture2D();
      break;
    case SubTests::ST_Texture2DArray:
      Texture2DArray();
      break;
    case SubTests::ST_GenerateMipMaps:
      GenerateMipMaps();
      break;
    case SubTests::ST_PushConstants:
      PushConstantsTest();
      break;
    case SubTests::ST_SetsSlots:
      SetsSlotsTest();
      break;
    case SubTests::ST_Timestamps:
    {
      auto res = Timestamps();
      EndFrame();
      return res;
    }
    break;
    case SubTests::ST_OcclusionQueries:
    {
      auto res = OcclusionQueries();
      EndFrame();
      return res;
    }
    break;
    case SubTests::ST_CustomVertexStreams:
      CustomVertexStreams();
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  EndFrame();

  if (m_ImgCompFrames.IsEmpty() || m_ImgCompFrames.PeekBack() == m_iFrame)
  {
    return ezTestAppRun::Quit;
  }
  return ezTestAppRun::Continue;
}

void ezRendererTestPipelineStates::MapImageNumberToString(const char* szTestName, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber, ezStringBuilder& out_sString) const
{
  if (subTest.m_iSubTestIdentifier == ST_ByteAddressBuffer || subTest.m_iSubTestIdentifier == ST_TexelBuffer)
  {
    out_sString.SetFormat("{0}_{1}_{2}", szTestName, GetSubTestName(ST_StructuredBuffer), ezArgI(uiImageNumber, 3, true));
    out_sString.ReplaceAll(" ", "_");
  }
  else
  {
    ezGraphicsTest::MapImageNumberToString(szTestName, subTest, uiImageNumber, out_sString);
  }
}

void ezRendererTestPipelineStates::RenderBlock(ezMeshBufferResourceHandle mesh, ezColor clearColor, ezUInt32 uiRenderTargetClearMask, ezRectFloat* pViewport, ezRectU32* pScissor)
{
  BeginCommands("MostBasicTriangle");
  {
    ezGALCommandEncoder* pCommandEncoder = BeginRendering(clearColor, uiRenderTargetClearMask, pViewport, pScissor);
    {

      if (mesh.IsValid())
      {
        ezRenderContext::GetDefaultInstance()->BindShader(m_hNDCPositionOnlyShader);
        ezRenderContext::GetDefaultInstance()->BindMeshBuffer(mesh);
        ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();
      }
      else
      {
        ezRenderContext::GetDefaultInstance()->BindShader(m_hMostBasicTriangleShader);
        ezRenderContext::GetDefaultInstance()->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);
        ezRenderContext::GetDefaultInstance()->DrawMeshBuffer(1).AssertSuccess();
      }
    }

    EndRendering();
    if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
    {
      EZ_TEST_IMAGE(m_iFrame, 100);
    }
  }

  EndCommands();
}



void ezRendererTestPipelineStates::MostBasicTriangleTest()
{
  m_bCaptureImage = true;
  RenderBlock({}, ezColor::RebeccaPurple);
}

void ezRendererTestPipelineStates::ViewportScissorTest()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezUInt32 uiColumns = 2;
  const ezUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
  RenderBlock({}, ezColor::CornflowerBlue, 0xFFFFFFFF, &viewport);

  viewport = ezRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
  RenderBlock({}, ezColor::Green, 0, &viewport);

  viewport = ezRectFloat(0, 0, fElementWidth, fHeight);
  ezRectU32 scissor = ezRectU32(0, (ezUInt32)fElementHeight, (ezUInt32)fElementWidth, (ezUInt32)fElementHeight);
  RenderBlock({}, ezColor::Green, 0, &viewport, &scissor);

  m_bCaptureImage = true;
  viewport = ezRectFloat(0, 0, fWidth, fHeight);
  scissor = ezRectU32((ezUInt32)fElementWidth, 0, (ezUInt32)fElementWidth, (ezUInt32)fElementHeight);
  RenderBlock({}, ezColor::Green, 0, &viewport, &scissor);
}

void ezRendererTestPipelineStates::VertexBufferTest()
{
  m_bCaptureImage = true;
  RenderBlock(m_hTriangleMesh, ezColor::RebeccaPurple);
}

void ezRendererTestPipelineStates::IndexBufferTest()
{
  m_bCaptureImage = true;
  RenderBlock(m_hSphereMesh, ezColor::Orange);
}

void ezRendererTestPipelineStates::PushConstantsTest()
{
  const ezUInt32 uiColumns = 4;
  const ezUInt32 uiRows = 2;

  BeginCommands("PushConstantsTest");
  {
    ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::CornflowerBlue, 0xFFFFFFFF);
    ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();
    {
      pContext->BindShader(m_hPushConstantsShader);
      pContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

      for (ezUInt32 x = 0; x < uiColumns; ++x)
      {
        for (ezUInt32 y = 0; y < uiRows; ++y)
        {
          ezTestData constants;
          ezTransform t = ezRendererTestUtils::CreateTransform(uiColumns, uiRows, x, y);
          constants.Vertex0 = (t * ezVec3(1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
          constants.Vertex1 = (t * ezVec3(-1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
          constants.Vertex2 = (t * ezVec3(-0.f, 1.f, 0.0f)).GetAsVec4(1.0f);
          constants.VertexColor = ezColorScheme::LightUI(float(x * uiRows + y) / (uiColumns * uiRows)).GetAsVec4();

          pContext->SetPushConstants("ezTestData", constants);
          pContext->DrawMeshBuffer(1).AssertSuccess();
        }
      }
    }
    EndRendering();
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      EZ_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}

void ezRendererTestPipelineStates::SetsSlotsTest()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezUInt32 uiColumns = 2;
  const ezUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const ezMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);

  auto constants = ezRenderContext::GetConstantBufferData<ezTestPerFrame>(m_hTestPerFrameConstantBuffer);
  constants->Time = 1.0f;
  ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();
  pContext->BindConstantBuffer("ezTestPerFrame", m_hTestPerFrameConstantBuffer);

  BeginCommands("SetsSlots");
  {
    ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D_Mip0);
    viewport = ezRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip1);
    viewport = ezRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip2);
    m_bCaptureImage = true;
    viewport = ezRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip3);
  }
  EndCommands();
}

void ezRendererTestPipelineStates::ConstantBufferTest()
{
  const ezUInt32 uiColumns = 4;
  const ezUInt32 uiRows = 2;

  BeginCommands("ConstantBufferTest");
  {
    ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::CornflowerBlue, 0xFFFFFFFF);
    ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();
    {
      pContext->BindConstantBuffer("ezTestColors", m_hTestColorsConstantBuffer);
      pContext->BindConstantBuffer("ezTestPositions", m_hTestPositionsConstantBuffer);
      pContext->BindShader(m_hConstantBufferShader);
      pContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, 1);

      for (ezUInt32 x = 0; x < uiColumns; ++x)
      {
        for (ezUInt32 y = 0; y < uiRows; ++y)
        {
          {
            auto constants = ezRenderContext::GetConstantBufferData<ezTestColors>(m_hTestColorsConstantBuffer);
            constants->VertexColor = ezColorScheme::LightUI(float(x * uiRows + y) / (uiColumns * uiRows)).GetAsVec4();
          }
          {
            ezTransform t = ezRendererTestUtils::CreateTransform(uiColumns, uiRows, x, y);
            auto constants = ezRenderContext::GetConstantBufferData<ezTestPositions>(m_hTestPositionsConstantBuffer);
            constants->Vertex0 = (t * ezVec3(1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
            constants->Vertex1 = (t * ezVec3(-1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
            constants->Vertex2 = (t * ezVec3(-0.f, 1.f, 0.0f)).GetAsVec4(1.0f);
          }
          pContext->DrawMeshBuffer(1).AssertSuccess();
        }
      }
    }
    EndRendering();
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      EZ_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}

void ezRendererTestPipelineStates::StructuredBufferTestUpload()
{
  if (m_iFrame == ImageCaptureFrames::StructuredBuffer_UpdateForNextFrame)
  {
    // Replace the elements at [0, 3] with more green ones by offsetting the color by 16.
    ezHybridArray<ezTestShaderData, 16> instanceData;
    ezRendererTestUtils::FillStructuredBuffer(instanceData, 16 /*green*/);
    m_pDevice->UpdateBufferForNextFrame(m_hInstancingData, instanceData.GetArrayPtr().GetSubArray(0, 4).ToByteArray());
  }
  if (m_iFrame == ImageCaptureFrames::StructuredBuffer_UpdateForNextFrame2)
  {
    // Replace the elements at [8, 15] with the same data as the original 8 elements. We will render these afterwards using custom buffer views.
    ezHybridArray<ezTestShaderData, 16> instanceData;
    ezRendererTestUtils::FillStructuredBuffer(instanceData);
    m_pDevice->UpdateBufferForNextFrame(m_hInstancingData, instanceData.GetArrayPtr().GetSubArray(0, 8).ToByteArray(), sizeof(ezTestShaderData) * 8);
  }
}

void ezRendererTestPipelineStates::StructuredBufferTest(ezGALShaderResourceType::Enum bufferType)
{
  BeginCommands("InstancingTest");
  {
    ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();

    if (m_iFrame == ImageCaptureFrames::StructuredBuffer_UAV)
    {
      pContext->BeginCompute("ComputeCopyData");

      pContext->BindShader(m_hCopyBufferShader);
      // Copy [12, 17] to the front (green)
      pContext->BindBuffer("instancingData", m_pDevice->GetDefaultResourceView(m_hInstancingData));
      pContext->BindUAV("instancingTarget", m_hInstancingDataUavView_0_4);
      pContext->Dispatch(4, 1, 1).AssertSuccess();
      // Copy [8, 11] to the back (red)
      pContext->BindBuffer("instancingData", m_hInstancingDataView_12_4);
      pContext->BindUAV("instancingTarget", m_hInstancingDataUavView_4_4);
      pContext->Dispatch(4, 1, 1).AssertSuccess();

      pContext->EndCompute();
    }

    ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::CornflowerBlue, 0xFFFFFFFF);
    {
      pContext->BindShader(m_hInstancingShader);
      pContext->BindMeshBuffer(m_hTriangleMesh);

      if (m_iFrame <= ImageCaptureFrames::StructuredBuffer_UpdateForNextFrame)
      {
        pContext->BindBuffer("instancingData", m_pDevice->GetDefaultResourceView(m_hInstancingData));
        pContext->DrawMeshBuffer(1, 0, 8).AssertSuccess();
      }
      else if (m_iFrame == ImageCaptureFrames::StructuredBuffer_UpdateForNextFrame2)
      {
        // Use the second half of the buffer to render the 8 triangles using two draw calls.
        pContext->BindBuffer("instancingData", m_hInstancingDataView_8_4);
        pContext->DrawMeshBuffer(1, 0, 4).AssertSuccess();
        pContext->BindBuffer("instancingData", m_hInstancingDataView_12_4);
        pContext->DrawMeshBuffer(1, 0, 4).AssertSuccess();
      }
      else if (m_iFrame == ImageCaptureFrames::StructuredBuffer_Transient1)
      {
        ezHybridArray<ezTestShaderData, 16> instanceData;
        ezRendererTestUtils::FillStructuredBuffer(instanceData, 16 /*green*/);
        // Update the entire buffer in lots of little upload calls with greener versions.
        for (ezUInt32 i = 0; i < 16; i++)
        {
          pCommandEncoder->UpdateBuffer(m_hInstancingDataTransient, i * sizeof(ezTestShaderData), instanceData.GetArrayPtr().GetSubArray(i, 1).ToByteArray(), ezGALUpdateMode::AheadOfTime);
        }
        pContext->BindBuffer("instancingData", m_pDevice->GetDefaultResourceView(m_hInstancingDataTransient, bufferType));
        pContext->DrawMeshBuffer(1, 0, 8).AssertSuccess();
      }
      else if (m_iFrame == ImageCaptureFrames::StructuredBuffer_Transient2)
      {
        ezHybridArray<ezTestShaderData, 16> instanceData;
        ezRendererTestUtils::FillStructuredBuffer(instanceData);
        // Update with one single update call for the first 8 elements matching the initial state.
        pCommandEncoder->UpdateBuffer(m_hInstancingDataTransient, 0, instanceData.GetArrayPtr().GetSubArray(0, 8).ToByteArray(), ezGALUpdateMode::AheadOfTime);
        pContext->BindBuffer("instancingData", m_pDevice->GetDefaultResourceView(m_hInstancingDataTransient, bufferType));
        pContext->DrawMeshBuffer(1, 0, 8).AssertSuccess();
      }
      else if (m_iFrame == ImageCaptureFrames::StructuredBuffer_UAV)
      {
        pContext->BindBuffer("instancingData", m_pDevice->GetDefaultResourceView(m_hInstancingDataUAV));
        pContext->DrawMeshBuffer(1, 0, 8).AssertSuccess();
      }
    }
  }
  EndRendering();
  if (m_ImgCompFrames.Contains(m_iFrame))
  {
    EZ_TEST_IMAGE(m_iFrame, 100);
  }
  EndCommands();
}

void ezRendererTestPipelineStates::Texture2D()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezUInt32 uiColumns = 2;
  const ezUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const ezMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);

  BeginCommands("Texture2D");
  {
    ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D_Mip0);
    viewport = ezRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip1);
    viewport = ezRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip2);
    m_bCaptureImage = true;
    viewport = ezRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip3);
  }
  EndCommands();
}

void ezRendererTestPipelineStates::Texture2DArray()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezUInt32 uiColumns = 2;
  const ezUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const ezMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);

  BeginCommands("Texture2DArray");
  {
    ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2DArray_Layer0_Mip0);
    viewport = ezRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray_Layer0_Mip1);
    viewport = ezRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray_Layer1_Mip0);
    m_bCaptureImage = true;
    viewport = ezRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray_Layer1_Mip1);
  }
  EndCommands();
}

void ezRendererTestPipelineStates::GenerateMipMaps()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezUInt32 uiColumns = 2;
  const ezUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const ezMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  BeginCommands("GenerateMipMaps");
  {
    ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
    m_pEncoder->GenerateMipMaps(m_pDevice->GetDefaultResourceView(m_hTexture2D));

    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D_Mip0);
    viewport = ezRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip1);
    viewport = ezRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip2);
    m_bCaptureImage = true;
    viewport = ezRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip3);
  }
  EndCommands();
}

ezTestAppRun ezRendererTestPipelineStates::Timestamps()
{
  BeginCommands("Timestamps");
  {
    ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0xFFFFFFFF);

    if (m_iFrame == 2)
    {
      m_CPUTime[0] = ezTime::Now();
      m_timestamps[0] = pCommandEncoder->InsertTimestamp();
    }
    ezRenderContext::GetDefaultInstance()->BindShader(m_hNDCPositionOnlyShader);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hSphereMesh);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    if (m_iFrame == 2)
      m_timestamps[1] = pCommandEncoder->InsertTimestamp();

    EndRendering();

    if (m_iFrame == 2)
    {
      m_hFence = pCommandEncoder->InsertFence();
      pCommandEncoder->Flush();
    }
  }
  EndCommands();


  if (m_iFrame >= 2)
  {
    // #TODO_VULKAN Our CPU / GPU timestamp calibration is not precise enough to allow comparing between zones reliably. Need to implement VK_KHR_calibrated_timestamps.
    const ezTime epsilon = ezTime::MakeFromMilliseconds(16);
    ezEnum<ezGALAsyncResult> fenceResult = m_pDevice->GetFenceResult(m_hFence);
    if (fenceResult == ezGALAsyncResult::Ready)
    {
      if (m_pDevice->GetTimestampResult(m_timestamps[0], m_GPUTime[0]) == ezGALAsyncResult::Ready && m_pDevice->GetTimestampResult(m_timestamps[1], m_GPUTime[1]) == ezGALAsyncResult::Ready)
      {
        m_CPUTime[1] = ezTime::Now();
        EZ_TEST_BOOL_MSG(m_CPUTime[0] <= (m_GPUTime[0] + epsilon), "%.4f < %.4f", m_CPUTime[0].GetMilliseconds(), m_GPUTime[0].GetMilliseconds());
        EZ_TEST_BOOL_MSG(m_GPUTime[0] <= m_GPUTime[1], "%.4f < %.4f", m_GPUTime[0].GetMilliseconds(), m_GPUTime[1].GetMilliseconds());
        EZ_TEST_BOOL_MSG(m_GPUTime[1] <= (m_CPUTime[1] + epsilon), "%.4f < %.4f", m_GPUTime[1].GetMilliseconds(), m_CPUTime[1].GetMilliseconds());
        ezTestFramework::GetInstance()->Output(ezTestOutput::Message, "Timestamp results received after %d frames or %.2f ms (%d frames after fence)", m_iFrame - 2, (ezTime::Now() - m_CPUTime[0]).GetMilliseconds(), m_iDelay);
        return ezTestAppRun::Quit;
      }
      else
      {
        m_iDelay++;
      }
    }
  }

  if (m_iFrame >= 100)
  {
    ezLog::Error("Timestamp results did not complete in 100 frames / {} seconds", (ezTime::Now() - m_CPUTime[0]).AsFloatInSeconds());
    return ezTestAppRun::Quit;
  }
  return ezTestAppRun::Continue;
}

ezTestAppRun ezRendererTestPipelineStates::OcclusionQueries()
{
  BeginCommands("OcclusionQueries");
  {
    ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0xFFFFFFFF);

    // #TODO_VULKAN Vulkan will assert if we don't render something bogus here. The reason is that occlusion queries must be started and stopped within the same render pass. However, as we start the render pass lazily within ezGALCommandEncoderImplVulkan::FlushDeferredStateChanges, the BeginOcclusionQuery call is actually still outside the render pass.
    ezRenderContext::GetDefaultInstance()->BindShader(m_hNDCPositionOnlyShader);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hTriangleMesh);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    if (m_iFrame == 2)
    {
      EZ_TEST_BOOL(m_queries[0].IsInvalidated());
      m_queries[0] = pCommandEncoder->BeginOcclusionQuery(ezGALQueryType::NumSamplesPassed);
      EZ_TEST_BOOL(!m_queries[0].IsInvalidated());
      pCommandEncoder->EndOcclusionQuery(m_queries[0]);

      EZ_TEST_BOOL(m_queries[1].IsInvalidated());
      m_queries[1] = pCommandEncoder->BeginOcclusionQuery(ezGALQueryType::AnySamplesPassed);
      EZ_TEST_BOOL(!m_queries[1].IsInvalidated());
      pCommandEncoder->EndOcclusionQuery(m_queries[1]);

      m_queries[2] = pCommandEncoder->BeginOcclusionQuery(ezGALQueryType::NumSamplesPassed);
    }
    else if (m_iFrame == 3)
    {
      m_queries[3] = pCommandEncoder->BeginOcclusionQuery(ezGALQueryType::AnySamplesPassed);
    }

    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hSphereMesh);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    if (m_iFrame == 2)
    {
      pCommandEncoder->EndOcclusionQuery(m_queries[2]);
    }
    else if (m_iFrame == 3)
    {
      pCommandEncoder->EndOcclusionQuery(m_queries[3]);
    }
    EndRendering();

    if (m_iFrame == 3)
    {
      m_CPUTime[0] = ezTime::Now();
      m_hFence = pCommandEncoder->InsertFence();
      pCommandEncoder->Flush();
    }
  }
  EndCommands();

  if (m_iFrame >= 3)
  {
    ezEnum<ezGALAsyncResult> fenceResult = m_pDevice->GetFenceResult(m_hFence);
    if (fenceResult == ezGALAsyncResult::Ready)
    {
      ezEnum<ezGALAsyncResult> queryResults[4];
      ezUInt64 queryValues[4];
      for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_queries); i++)
      {
        queryResults[i] = m_pDevice->GetOcclusionQueryResult(m_queries[i], queryValues[i]);
        if (!EZ_TEST_BOOL(queryResults[i] != ezGALAsyncResult::Expired))
        {
          return ezTestAppRun::Quit;
        }
      }

      bool bAllReady = true;
      for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_queries); i++)
      {
        if (queryResults[i] != ezGALAsyncResult::Ready)
          bAllReady = false;
      }

      if (bAllReady)
      {
        ezTestFramework::GetInstance()->Output(ezTestOutput::Message, "Occlusion query results received after %d frames or %.2f ms (%d frames after fence)", m_iFrame - 3, (ezTime::Now() - m_CPUTime[0]).GetMilliseconds(), m_iDelay);

        EZ_TEST_INT(queryValues[0], 0);
        EZ_TEST_INT(queryValues[1], 0);

        EZ_TEST_BOOL(queryValues[2] >= 1);
        EZ_TEST_BOOL(queryValues[3] >= 1);
        return ezTestAppRun::Quit;
      }
      else
      {
        m_iDelay++;
      }
    }
  }

  if (m_iFrame >= 100)
  {
    ezLog::Error("Occlusion query results did not complete in 100 frames / {} seconds", (ezTime::Now() - m_CPUTime[0]).AsFloatInSeconds());
    return ezTestAppRun::Quit;
  }

  return ezTestAppRun::Continue;
}

void ezRendererTestPipelineStates::CustomVertexStreams()
{
  BeginCommands("InstancingTest");
  {
    ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::CornflowerBlue, 0xFFFFFFFF);

    ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();
    {
      pContext->BindShader(m_hCustomVertexStreamShader);
      pContext->BindMeshBuffer(m_hTriangleMesh);
      pContext->SetCustomVertexStreams(m_CustomVertexStreams);

      if (m_iFrame <= ImageCaptureFrames::DefaultCapture)
      {
        pContext->BindVertexBuffer(m_hInstancingDataCustomVertexStream, 5, ezGALVertexBindingRate::Instance, 0);
        pContext->DrawMeshBuffer(1, 0, 8).AssertSuccess();
      }
      else if (m_iFrame >= ImageCaptureFrames::CustomVertexStreams_Offsets)
      {
        // Render the same image but this time using two draw calls with offsets.
        pContext->BindVertexBuffer(m_hInstancingDataCustomVertexStream, 5, ezGALVertexBindingRate::Instance, 0);
        pContext->DrawMeshBuffer(1, 0, 4).AssertSuccess();
        pContext->BindVertexBuffer(m_hInstancingDataCustomVertexStream, 5, ezGALVertexBindingRate::Instance, 4 * sizeof(ezTestShaderData));
        pContext->DrawMeshBuffer(1, 0, 4).AssertSuccess();
      }

      // Make sure to reset once no longer in use, or it will mess with subsequent rendering.
      pContext->SetCustomVertexStreams({});
    }
  }
  EndRendering();
  if (m_ImgCompFrames.Contains(m_iFrame))
  {
    EZ_TEST_IMAGE(m_iFrame, 100);
  }
  EndCommands();
}

static ezRendererTestPipelineStates g_PipelineStatesTest;
