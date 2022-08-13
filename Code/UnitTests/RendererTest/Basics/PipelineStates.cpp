#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <RendererTest/Basics/PipelineStates.h>

#include <RendererTest/../../../Data/UnitTests/RendererTest/Shaders/TestConstants.h>
#include <RendererTest/../../../Data/UnitTests/RendererTest/Shaders/TestInstancing.h>

EZ_DEFINE_AS_POD_TYPE(ezTestShaderData);

namespace
{
  ezTransform CreateTransform(const ezUInt32 uiColumns, const ezUInt32 uiRows, ezUInt32 x, ezUInt32 y)
  {
    ezTransform t = ezTransform::IdentityTransform();
    t.m_vScale = ezVec3(1.0f / float(uiColumns), 1.0f / float(uiRows), 1);
    t.m_vPosition = ezVec3(ezMath::Lerp(-1.f, 1.f, (float(x) + 0.5f) / float(uiColumns)), ezMath::Lerp(1.f, -1.f, (float(y) + 0.5f) / float(uiRows)), 0);
    if (ezClipSpaceYMode::RenderToTextureDefault == ezClipSpaceYMode::Flipped)
    {
      ezTransform flipY = ezTransform::IdentityTransform();
      flipY.m_vScale.y *= -1.0f;
      t = flipY * t;
    }
    return t;
  }

  void FillStructuredBuffer(ezHybridArray<ezTestShaderData, 16>& instanceData, ezUInt32 uiColorOffset = 0, ezUInt32 uiSlotOffset = 0)
  {
    instanceData.SetCount(16);
    const ezUInt32 uiColumns = 4;
    const ezUInt32 uiRows = 2;

    for (ezUInt32 x = 0; x < uiColumns; ++x)
    {
      for (ezUInt32 y = 0; y < uiRows; ++y)
      {
        ezTestShaderData& instance = instanceData[uiSlotOffset + x * uiRows + y];
        instance.InstanceColor = ezColor::GetPaletteColor(uiColorOffset + x * uiRows + y).GetAsVec4();
        ezTransform t = CreateTransform(uiColumns, uiRows, x, y);
        instance.InstanceTransform = t;
      }
    }
  }

  struct ImgColor
  {
    EZ_DECLARE_POD_TYPE();
    ezUInt8 b;
    ezUInt8 g;
    ezUInt8 r;
    ezUInt8 a;
  };

  void CreateImage(ezImage& image, ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiMipLevelCount, bool bMipLevelIsBlue, ezUInt8 uiFixedBlue = 0)
  {
    ezImageHeader header;
    header.SetImageFormat(ezImageFormat::B8G8R8A8_UNORM_SRGB);
    header.SetWidth(uiWidth);
    header.SetHeight(uiHeight);
    header.SetNumMipLevels(uiMipLevelCount);

    image.ResetAndAlloc(header);
    for (ezUInt32 m = 0; m < uiMipLevelCount; m++)
    {
      const ezUInt32 uiHeight = image.GetHeight(m);
      const ezUInt32 uiWidth = image.GetWidth(m);

      const ezUInt8 uiBlue = bMipLevelIsBlue ? static_cast<ezUInt8>(255.0f * float(m) / (uiMipLevelCount - 1)) : uiFixedBlue;
      for (ezUInt32 y = 0; y < uiHeight; y++)
      {
        const ezUInt8 uiGreen = static_cast<ezUInt8>(255.0f * float(y) / (uiHeight - 1));
        for (ezUInt32 x = 0; x < uiWidth; x++)
        {
          ImgColor* pColor = image.GetPixelPointer<ImgColor>(m, 0u, 0u, x, y);
          pColor->a = 255;
          pColor->b = uiBlue;
          pColor->g = uiGreen;
          pColor->r = static_cast<ezUInt8>(255.0f * float(x) / (uiWidth - 1));
        }
      }
    }
  }

  ezMat4 CreateSimpleMVP(float fAspectRatio)
  {
    ezCamera cam;
    cam.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90, 0.5f, 1000.0f);
    cam.LookAt(ezVec3(0, 0, 0), ezVec3(0, 0, -1), ezVec3(0, 1, 0));
    ezMat4 mProj;
    cam.GetProjectionMatrix(fAspectRatio, mProj);
    ezMat4 mView = cam.GetViewMatrix();

    ezMat4 mTransform;
    mTransform.SetTranslationMatrix(ezVec3(0.0f, 0.0f, -1.2f));
    return mProj * mView * mTransform;
  }
} // namespace



ezResult ezRendererTestPipelineStates::InitializeSubTest(ezInt32 iIdentifier)
{
  m_iFrame = -1;
  m_bCaptureImage = false;
  m_ImgCompFrames.Clear();

  {
    m_bTimestampsValid = false;
    m_CPUTime[0] = {};
    m_CPUTime[1] = {};
    m_GPUTime[0] = {};
    m_GPUTime[1] = {};
    m_timestamps[0] = {};
    m_timestamps[1] = {};
  }

  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));
  EZ_SUCCEED_OR_RETURN(SetupRenderer());
  EZ_SUCCEED_OR_RETURN(CreateWindow(320, 240));
  m_hMostBasicTriangleShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/MostBasicTriangle.ezShader");
  m_hNDCPositionOnlyShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/NDCPositionOnly.ezShader");
  m_hConstantBufferShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/ConstantBuffer.ezShader");
  m_hInstancingShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Instancing.ezShader");

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
    geom.AddSphere(0.5f, 16, 16);

    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

    m_hSphereMesh = ezResourceManager::CreateResource<ezMeshBufferResource>("UnitTest-SphereMesh", std::move(desc), "SphereMesh");
  }
  m_hTestColorsConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezTestColors>();
  m_hTestPositionsConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezTestPositions>();

  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezTestShaderData);
    desc.m_uiTotalSize = 16 * desc.m_uiStructSize;
    desc.m_BufferType = ezGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    // We only fill the first 8 elements with data. The rest is dynamically updated during testing.
    ezHybridArray<ezTestShaderData, 16> instanceData;
    FillStructuredBuffer(instanceData);
    m_hInstancingData = m_pDevice->CreateBuffer(desc, instanceData.GetByteArrayPtr());

    ezGALResourceViewCreationDescription viewDesc;
    viewDesc.m_hBuffer = m_hInstancingData;
    viewDesc.m_uiFirstElement = 8;
    viewDesc.m_uiNumElements = 4;
    m_hInstancingDataView_8_4 = m_pDevice->CreateResourceView(viewDesc);
    viewDesc.m_uiFirstElement = 12;
    m_hInstancingDataView_12_4 = m_pDevice->CreateResourceView(viewDesc);
  }

  {
    // Texture2D
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = 8;
    desc.m_uiHeight = 8;
    desc.m_uiMipLevelCount = 4;
    desc.m_Format = ezGALResourceFormat::BGRAUByteNormalizedsRGB;

    ezImage coloredMips;
    CreateImage(coloredMips, desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, true);

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
            ImgColor* pColor = coloredMips.GetPixelPointer<ImgColor>(m, 0u, 0u, x, y);
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
      memoryDesc.m_pData = coloredMips.GetPixelPointer<ezUInt8>(m);
      memoryDesc.m_uiRowPitch = static_cast<ezUInt32>(coloredMips.GetRowPitch(m));
      memoryDesc.m_uiSlicePitch = static_cast<ezUInt32>(coloredMips.GetDepthPitch(m));
    }
    m_hTexture2D = m_pDevice->CreateTexture(desc, initialData);

    ezGALResourceViewCreationDescription viewDesc;
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
    //Texture2DArray
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = 8;
    desc.m_uiHeight = 8;
    desc.m_uiMipLevelCount = 4;
    desc.m_uiArraySize = 2;
    desc.m_Type = ezGALTextureType::Texture2D;
    desc.m_Format = ezGALResourceFormat::BGRAUByteNormalizedsRGB;

    ezImage coloredMips[2];
    CreateImage(coloredMips[0], desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, false, 0);
    CreateImage(coloredMips[1], desc.m_uiWidth, desc.m_uiHeight, desc.m_uiMipLevelCount, false, 255);

    ezHybridArray<ezGALSystemMemoryDescription, 8> initialData;
    initialData.SetCount(desc.m_uiArraySize * desc.m_uiMipLevelCount);
    for (ezUInt32 l = 0; l < desc.m_uiArraySize; l++)
    {
      for (ezUInt32 m = 0; m < desc.m_uiMipLevelCount; m++)
      {
        ezGALSystemMemoryDescription& memoryDesc = initialData[m + l * desc.m_uiMipLevelCount];
        memoryDesc.m_pData = coloredMips[l].GetPixelPointer<ezUInt8>(m);
        memoryDesc.m_uiRowPitch = static_cast<ezUInt32>(coloredMips[l].GetRowPitch(m));
        memoryDesc.m_uiSlicePitch = static_cast<ezUInt32>(coloredMips[l].GetDepthPitch(m));
      }
    }
    m_hTexture2DArray = m_pDevice->CreateTexture(desc, initialData);

    ezGALResourceViewCreationDescription viewDesc;
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

  {
    // Cube mesh
    ezGeometry geom;
    geom.AddBox(ezVec3(1.0f), true);

    ezGALPrimitiveTopology::Enum Topology = ezGALPrimitiveTopology::Triangles;
    ezMeshBufferResourceDescriptor desc;
    desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
    desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::RGFloat);
    desc.AllocateStreamsFromGeometry(geom, Topology);

    m_hCubeUV = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>("Texture2DBox", std::move(desc), "Texture2DBox");
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
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_InitialData);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_Discard);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_NoOverwrite);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::StructuredBuffer_CopyToTempStorage);
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
    case SubTests::ST_Timestamps:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::Timestamps_MaxWaitTime);
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
  m_hInstancingShader.Invalidate();

  m_hTestColorsConstantBuffer.Invalidate();
  m_hTestPositionsConstantBuffer.Invalidate();

  if (!m_hInstancingData.IsInvalidated())
  {
    m_pDevice->DestroyBuffer(m_hInstancingData);
    m_hInstancingData.Invalidate();
  }
  m_hInstancingDataView_8_4.Invalidate();
  m_hInstancingDataView_12_4.Invalidate();

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
  m_hCubeUV.Invalidate();
  m_hShader.Invalidate();

  DestroyWindow();
  ShutdownRenderer();
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::DeInitializeSubTest(iIdentifier));
  return EZ_SUCCESS;
}

ezTestAppRun ezRendererTestPipelineStates::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;
  m_bCaptureImage = false;
  m_pDevice->BeginFrame(uiInvocationCount);
  m_pDevice->BeginPipeline("GraphicsTest", m_hSwapChain);

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
      StructuredBufferTest();
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
    case SubTests::ST_Timestamps:
      Timestamps();
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  ezRenderContext::GetDefaultInstance()->ResetContextState();
  m_pDevice->EndPipeline(m_hSwapChain);
  m_pDevice->EndFrame();

  ezTaskSystem::FinishFrameTasks();

  if (m_ImgCompFrames.IsEmpty() || m_ImgCompFrames.PeekBack() == m_iFrame)
  {
    return ezTestAppRun::Quit;
  }
  return ezTestAppRun::Continue;
}

void ezRendererTestPipelineStates::RenderBlock(ezMeshBufferResourceHandle mesh, ezColor clearColor, ezUInt32 uiRenderTargetClearMask, ezRectFloat* pViewport, ezRectU32* pScissor)
{
  m_pPass = m_pDevice->BeginPass("MostBasicTriangle");
  {
    ezGALRenderCommandEncoder* pCommandEncoder = BeginRendering(clearColor, uiRenderTargetClearMask, pViewport, pScissor);
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

    if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
    {
      EZ_TEST_IMAGE(m_iFrame, 100);
    }
    EndRendering();
  }

  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

ezGALRenderCommandEncoder* ezRendererTestPipelineStates::BeginRendering(ezColor clearColor, ezUInt32 uiRenderTargetClearMask, ezRectFloat* pViewport, ezRectU32* pScissor)
{
  const ezGALSwapChain* pPrimarySwapChain = m_pDevice->GetSwapChain(m_hSwapChain);

  ezGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(pPrimarySwapChain->GetBackBufferTexture()));
  renderingSetup.m_ClearColor = clearColor;
  renderingSetup.m_uiRenderTargetClearMask = uiRenderTargetClearMask;
  if (!m_hDepthStencilTexture.IsInvalidated())
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hDepthStencilTexture));
    renderingSetup.m_bClearDepth = true;
    renderingSetup.m_bClearStencil = true;
  }
  ezRectFloat viewport = ezRectFloat(0.0f, 0.0f, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);
  if (pViewport)
  {
    viewport = *pViewport;
  }

  ezGALRenderCommandEncoder* pCommandEncoder = ezRenderContext::GetDefaultInstance()->BeginRendering(m_pPass, renderingSetup, viewport);
  ezRectU32 scissor = ezRectU32(0, 0, m_pWindow->GetClientAreaSize().width, m_pWindow->GetClientAreaSize().height);
  if (pScissor)
  {
    scissor = *pScissor;
  }
  pCommandEncoder->SetScissorRect(scissor);

  SetClipSpace();
  return pCommandEncoder;
}

void ezRendererTestPipelineStates::EndRendering()
{
  ezRenderContext::GetDefaultInstance()->EndRendering();
  m_pWindow->ProcessWindowMessages();
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

void ezRendererTestPipelineStates::ConstantBufferTest()
{
  const ezUInt32 uiColumns = 4;
  const ezUInt32 uiRows = 2;

  m_pPass = m_pDevice->BeginPass("ConstantBufferTest");
  {
    ezGALRenderCommandEncoder* pCommandEncoder = BeginRendering(ezColor::CornflowerBlue, 0xFFFFFFFF);
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
            constants->VertexColor = ezColor::GetPaletteColor(x * uiRows + y).GetAsVec4();
          }
          {
            ezTransform t = CreateTransform(uiColumns, uiRows, x, y);
            auto constants = ezRenderContext::GetConstantBufferData<ezTestPositions>(m_hTestPositionsConstantBuffer);
            constants->Vertex0 = (t * ezVec3(1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
            constants->Vertex1 = (t * ezVec3(-1.f, -1.f, 0.0f)).GetAsVec4(1.0f);
            constants->Vertex2 = (t * ezVec3(-0.f, 1.f, 0.0f)).GetAsVec4(1.0f);
          }
          pContext->DrawMeshBuffer(1).AssertSuccess();
        }
      }
    }
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      EZ_TEST_IMAGE(m_iFrame, 100);
    }
    EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}


void ezRendererTestPipelineStates::StructuredBufferTest()
{
  m_pPass = m_pDevice->BeginPass("InstancingTest");
  {

    ezGALRenderCommandEncoder* pCommandEncoder = BeginRendering(ezColor::CornflowerBlue, 0xFFFFFFFF);
    if (m_iFrame == ImageCaptureFrames::StructuredBuffer_Discard)
    {
      // Discard previous buffer.
      ezHybridArray<ezTestShaderData, 16> instanceData;
      FillStructuredBuffer(instanceData, 16);
      pCommandEncoder->UpdateBuffer(m_hInstancingData, 0, instanceData.GetArrayPtr().ToByteArray(), ezGALUpdateMode::Discard);
    }
    else if (m_iFrame == ImageCaptureFrames::StructuredBuffer_NoOverwrite)
    {
      // Nothing has touched the second half of the new buffer yet. Fill it with the original data of the first 8 elements.
      ezHybridArray<ezTestShaderData, 16> instanceData;
      FillStructuredBuffer(instanceData);
      instanceData.SetCount(8);
      pCommandEncoder->UpdateBuffer(m_hInstancingData, 8 * sizeof(ezTestShaderData), instanceData.GetArrayPtr().ToByteArray(), ezGALUpdateMode::NoOverwrite);
    }
    else if (m_iFrame == ImageCaptureFrames::StructuredBuffer_CopyToTempStorage)
    {
      // Now we replace the first 4 elements of the second half of the buffer.
      ezHybridArray<ezTestShaderData, 16> instanceData;
      FillStructuredBuffer(instanceData, 16);
      instanceData.SetCount(4);
      pCommandEncoder->UpdateBuffer(m_hInstancingData, 8 * sizeof(ezTestShaderData), instanceData.GetArrayPtr().ToByteArray(), ezGALUpdateMode::CopyToTempStorage);
    }

    ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();
    {
      pContext->BindShader(m_hInstancingShader);
      pContext->BindMeshBuffer(m_hTriangleMesh);

      if (m_iFrame < ImageCaptureFrames::StructuredBuffer_NoOverwrite)
      {
        pContext->BindBuffer("instancingData", m_pDevice->GetDefaultResourceView(m_hInstancingData));
        pContext->DrawMeshBuffer(1, 0, 8).AssertSuccess();
      }
      else if (m_iFrame >= ImageCaptureFrames::StructuredBuffer_NoOverwrite)
      {
        pContext->BindBuffer("instancingData", m_hInstancingDataView_8_4);
        pContext->DrawMeshBuffer(1, 0, 4).AssertSuccess();
        pContext->BindBuffer("instancingData", m_hInstancingDataView_12_4);
        pContext->DrawMeshBuffer(1, 0, 4).AssertSuccess();
      }
    }
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      EZ_TEST_IMAGE(m_iFrame, 100);
    }
    EndRendering();
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

void ezRendererTestPipelineStates::RenderCube(ezRectFloat viewport, ezMat4 mMVP, ezUInt32 uiRenderTargetClearMask, ezGALResourceViewHandle hSRV)
{
  ezGALRenderCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, uiRenderTargetClearMask, &viewport);

  ezRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", hSRV);
  RenderObject(m_hCubeUV, mMVP, ezColor(1, 1, 1, 1), ezShaderBindFlags::None);
  if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
  {
    EZ_TEST_IMAGE(m_iFrame, 100);
  }
  EndRendering();
};

void ezRendererTestPipelineStates::Texture2D()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezUInt32 uiColumns = 2;
  const ezUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const ezMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);

  m_pPass = m_pDevice->BeginPass("Texture2D");
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
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
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

  m_pPass = m_pDevice->BeginPass("Texture2DArray");
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
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
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
  m_pPass = m_pDevice->BeginPass("GenerateMipMaps");
  {
    ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
    ezGALRenderCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0, &viewport);
    pCommandEncoder->GenerateMipMaps(m_pDevice->GetDefaultResourceView(m_hTexture2D));
    EndRendering();

    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D_Mip0);
    viewport = ezRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip1);
    viewport = ezRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip2);
    m_bCaptureImage = true;
    viewport = ezRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D_Mip3);
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;
}

void ezRendererTestPipelineStates::Timestamps()
{
  m_pPass = m_pDevice->BeginPass("Timestamps");
  {
    ezGALRenderCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0xFFFFFFFF);

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
  }
  m_pDevice->EndPass(m_pPass);
  m_pPass = nullptr;

  if (m_iFrame > 2 && !m_bTimestampsValid)
  {
    if ((m_bTimestampsValid = m_pDevice->GetTimestampResult(m_timestamps[0], m_GPUTime[0]).Succeeded() && m_pDevice->GetTimestampResult(m_timestamps[1], m_GPUTime[1]).Succeeded()))
    {
      m_CPUTime[1] = ezTime::Now();
      EZ_TEST_BOOL_MSG(m_CPUTime[0] <= m_GPUTime[0], "%.6f < %.6f", m_CPUTime[0].GetSeconds(), m_GPUTime[0].GetSeconds());
      EZ_TEST_BOOL_MSG(m_GPUTime[0] <= m_GPUTime[1], "%.6f < %.6f", m_GPUTime[0].GetSeconds(), m_GPUTime[1].GetSeconds());
      EZ_TEST_BOOL_MSG(m_GPUTime[1] <= m_CPUTime[1], "%.6f < %.6f", m_GPUTime[1].GetSeconds(), m_CPUTime[1].GetSeconds());
      ezTestFramework::GetInstance()->Output(ezTestOutput::Message, "Timestamp results received after %d frames and %.3f seconds.", m_iFrame, (ezTime::Now() - m_CPUTime[0]).AsFloatInSeconds());
      m_ImgCompFrames.Clear();
    }
  }
  ezThreadUtils::Sleep(ezTime::Milliseconds(16));
  if (m_iFrame > 2 && (ezTime::Now() - m_CPUTime[0]).AsFloatInSeconds() > 10.0f)
  {
    EZ_TEST_BOOL_MSG(m_bTimestampsValid, "Timestamp results are not present after 10 seconds.");
    m_ImgCompFrames.Clear();
  }
}

static ezRendererTestPipelineStates g_PipelineStatesTest;
