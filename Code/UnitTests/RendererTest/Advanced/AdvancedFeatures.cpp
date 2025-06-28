#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererTest/Advanced/AdvancedFeatures.h>
#include <RendererTest/Basics/RendererTestUtils.h>
#undef CreateWindow


void ezRendererTestAdvancedFeatures::SetupSubTests()
{
  const ezGALDeviceCapabilities& caps = GetDeviceCapabilities();

  AddSubTest("01 - ReadRenderTarget", SubTests::ST_ReadRenderTarget);
  if (caps.m_bSupportsVSRenderTargetArrayIndex)
  {
    AddSubTest("02 - VertexShaderRenderTargetArrayIndex", SubTests::ST_VertexShaderRenderTargetArrayIndex);
  }
#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
  if (caps.m_bSupportsSharedTextures)
  {
    AddSubTest("03 - SharedTexture", SubTests::ST_SharedTexture);
  }
#endif

  if (caps.m_bShaderStageSupported[ezGALShaderStage::HullShader])
  {
    AddSubTest("04 - Tessellation", SubTests::ST_Tessellation);
  }
  if (caps.m_bShaderStageSupported[ezGALShaderStage::ComputeShader])
  {
    AddSubTest("05 - Compute", SubTests::ST_Compute);
  }
  AddSubTest("06 - FloatSampling", SubTests::ST_FloatSampling);
  AddSubTest("07 - ProxyTexture", SubTests::ST_ProxyTexture);
  AddSubTest("08 - Material", SubTests::ST_Material);
}

ezResult ezRendererTestAdvancedFeatures::InitializeSubTest(ezInt32 iIdentifier)
{
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::InitializeSubTest(iIdentifier));
  EZ_SUCCEED_OR_RETURN(CreateWindow(320, 240));

  if (iIdentifier == ST_ReadRenderTarget)
  {
    // Texture2D
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, ezGALResourceFormat::BGRAUByteNormalizedsRGB, ezGALMSAASampleCount::None);
    m_hTexture2D = m_pDevice->CreateTexture(desc);

    m_Texture2DRange = {};
    m_Texture2DRange.m_uiMipLevels = 1;
    m_Texture2DRange.m_uiBaseMipLevel = 0;

    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/UVColor.ezShader");
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2D.ezShader");
  }

  if (iIdentifier == ST_ProxyTexture)
  {
    // Texture2DArray
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, ezGALResourceFormat::BGRAUByteNormalizedsRGB, ezGALMSAASampleCount::None);
    desc.m_Type = ezGALTextureType::Texture2DArray;
    desc.m_uiArraySize = 2;
    m_hTexture2DArray = m_pDevice->CreateTexture(desc);

    // Proxy texture
    m_hProxyTexture2D[0] = m_pDevice->CreateProxyTexture(m_hTexture2DArray, 0);
    m_hProxyTexture2D[1] = m_pDevice->CreateProxyTexture(m_hTexture2DArray, 1);

    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2D.ezShader");
    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/UVColor.ezShader");
    m_hShader3 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/UVColor2.ezShader");
  }

  if (iIdentifier == ST_FloatSampling)
  {
    // Texture2DArray
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, ezGALResourceFormat::D16, ezGALMSAASampleCount::None);
    desc.m_Type = ezGALTextureType::Texture2DArray;
    m_hTexture2DArray = m_pDevice->CreateTexture(desc);

    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/ReadbackDepth.ezShader");
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/SampleLevel_PointClampBorder.ezShader");

    ezGALSamplerStateCreationDescription samplerDesc;
    samplerDesc.m_MinFilter = ezGALTextureFilterMode::Point;
    samplerDesc.m_MagFilter = ezGALTextureFilterMode::Point;
    samplerDesc.m_MipFilter = ezGALTextureFilterMode::Point;
    samplerDesc.m_AddressU = ezImageAddressMode::ClampBorder;
    samplerDesc.m_AddressV = ezImageAddressMode::ClampBorder;
    samplerDesc.m_AddressW = ezImageAddressMode::ClampBorder;
    samplerDesc.m_BorderColor = ezColor::White;

    m_hDepthSamplerState = ezGALDevice::GetDefaultDevice()->CreateSamplerState(samplerDesc);
  }

  if (iIdentifier == ST_Compute)
  {
    // Texture2D as compute RW target. Note that SRGB and depth formats are not supported by most graphics cards for this purpose.
    ezEnum<ezGALResourceFormat> textureFormat;
    ezGALResourceFormat::Enum formats[] = {ezGALResourceFormat::RGBAFloat, ezGALResourceFormat::BGRAUByteNormalized, ezGALResourceFormat::RGBAUByteNormalized};
    for (auto format : formats)
    {
      if (m_pDevice->GetCapabilities().m_FormatSupport[format].IsSet(ezGALResourceFormatSupport::TextureRW))
      {
        textureFormat = format;
        break;
      }
    }
    if (!EZ_TEST_BOOL(textureFormat != ezGALResourceFormat::Invalid))
      return EZ_FAILURE;

    // We are only rendering to mip map level 4 (8x8).
    // The array and levels and mip size is only here to test sub-resource views and Nvidia bugs (image will be too dark): https://forums.developer.nvidia.com/t/vulkan-driver-bug-regression-in-hlsl-getdimensions-on-rwtexture2darray/315282
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(128, 128, textureFormat, ezGALMSAASampleCount::None);
    desc.m_Type = ezGALTextureType::Texture2DArray;
    desc.m_bAllowUAV = true;
    desc.m_uiArraySize = 2;
    desc.m_bAllowRenderTargetView = false;
    desc.m_uiMipLevelCount = 6;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hTexture2D = m_pDevice->CreateTexture(desc);

    m_Texture2DRange = {};
    m_Texture2DRange.m_uiBaseMipLevel = 4;
    m_Texture2DRange.m_uiMipLevels = 1;
    m_Texture2DRange.m_uiBaseArraySlice = 0;
    m_Texture2DRange.m_uiArraySlices = 1;

    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/UVColorCompute.ezShader");
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2DReadbackDepth.ezShader");
  }

  if (iIdentifier == ST_VertexShaderRenderTargetArrayIndex)
  {
    // Texture2DArray
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(320 / 2, 240, ezGALResourceFormat::BGRAUByteNormalizedsRGB, ezGALMSAASampleCount::None);
    desc.m_Type = ezGALTextureType::Texture2DArray;
    desc.m_uiArraySize = 2;
    m_hTexture2DArray = m_pDevice->CreateTexture(desc);

    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Stereo.ezShader");
    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/StereoPreview.ezShader");
  }

  if (iIdentifier == ST_Material)
  {
    // Texture Resource
    ezGALTextureCreationDescription galTexDesc;
    galTexDesc.m_uiWidth = 8;
    galTexDesc.m_uiHeight = 8;
    galTexDesc.m_uiMipLevelCount = 1;
    galTexDesc.m_Format = ezGALResourceFormat::BGRAUByteNormalizedsRGB;

    ezImage coloredMips;
    ezRendererTestUtils::CreateImage(coloredMips, galTexDesc.m_uiWidth, galTexDesc.m_uiHeight, 1, true);

    ezHybridArray<ezGALSystemMemoryDescription, 1> initialData;
    initialData.SetCount(galTexDesc.m_uiMipLevelCount);
    for (ezUInt32 m = 0; m < galTexDesc.m_uiMipLevelCount; m++)
    {
      ezGALSystemMemoryDescription& memoryDesc = initialData[m];
      memoryDesc.m_pData = coloredMips.GetSubImageView(m).GetByteBlobPtr();
      memoryDesc.m_uiRowPitch = static_cast<ezUInt32>(coloredMips.GetRowPitch(m));
      memoryDesc.m_uiSlicePitch = static_cast<ezUInt32>(coloredMips.GetDepthPitch(m));
    }

    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/TestMaterial.ezShader");

    ezGALSamplerStateCreationDescription samplerDesc;
    samplerDesc.m_MinFilter = ezGALTextureFilterMode::Point;
    samplerDesc.m_MagFilter = ezGALTextureFilterMode::Point;
    samplerDesc.m_MipFilter = ezGALTextureFilterMode::Point;
    samplerDesc.m_AddressU = ezImageAddressMode::ClampBorder;
    samplerDesc.m_AddressV = ezImageAddressMode::ClampBorder;
    samplerDesc.m_AddressW = ezImageAddressMode::ClampBorder;
    samplerDesc.m_BorderColor = ezColor::White;

    ezTexture2DResourceDescriptor texDesc;
    texDesc.m_DescGAL = galTexDesc;
    texDesc.m_InitialContent = initialData;
    texDesc.m_SamplerDesc = samplerDesc;

    m_hTexture = ezResourceManager::LoadResource<ezTexture2DResource>("White.color");
    m_hTexture2 = ezResourceManager::CreateResource<ezTexture2DResource>("TestTexture", std::move(texDesc), "A Test Texture");

    // Material
    ezMaterialResourceDescriptor matDesc;
    matDesc.m_hShader = m_hShader;
    matDesc.m_RenderDataCategory = ezDefaultRenderDataCategories::LitOpaque;
    m_sBaseColor.Assign("BaseColor");
    m_sBaseColor2.Assign("BaseColor2");
    matDesc.m_Parameters.PushBack({m_sBaseColor, ezColor::White});
    matDesc.m_Parameters.PushBack({m_sBaseColor2, ezColor::White});

    m_sTexture.Assign("DiffuseTexture");
    matDesc.m_Texture2DBindings.PushBack({m_sTexture, m_hTexture});

    m_hMaterial = ezResourceManager::CreateResource<ezMaterialResource>("TestMaterial", std::move(matDesc), "A Test Material");
  }

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
  if (iIdentifier == ST_SharedTexture)
  {
    ezCVarFloat* pProfilingThreshold = (ezCVarFloat*)ezCVar::FindCVarByName("Profiling.DiscardThresholdMS");
    EZ_ASSERT_DEBUG(pProfilingThreshold, "Profiling.cpp cvar was renamed");
    m_fOldProfilingThreshold = *pProfilingThreshold;
    *pProfilingThreshold = 0.0f;

    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2D.ezShader");

    const ezStringBuilder pathToSelf = ezCommandLineUtils::GetGlobalInstance()->GetParameter(0);

    ezProcessOptions opt;
    opt.m_sProcess = pathToSelf;

    ezStringBuilder sIPC;
    ezConversionUtils::ToString(ezUuid::MakeUuid(), sIPC);

    ezStringBuilder sPID;
    ezConversionUtils::ToString(ezProcess::GetCurrentProcessID(), sPID);

#  ifdef BUILDSYSTEM_ENABLE_VULKAN_SUPPORT
    constexpr const char* szDefaultRenderer = "Vulkan";
#  else
    constexpr const char* szDefaultRenderer = "DX11";
#  endif
    ezStringView sRendererName = ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-renderer", 0, szDefaultRenderer);

    opt.m_Arguments.PushBack("-offscreen");
    opt.m_Arguments.PushBack("-IPC");
    opt.m_Arguments.PushBack(sIPC);
    opt.m_Arguments.PushBack("-PID");
    opt.m_Arguments.PushBack(sPID);
    opt.m_Arguments.PushBack("-renderer");
    opt.m_Arguments.PushBack(sRendererName);
    opt.m_Arguments.PushBack("-outputDir");
    opt.m_Arguments.PushBack(ezTestFramework::GetInstance()->GetAbsOutputPath());


    m_pOffscreenProcess = EZ_DEFAULT_NEW(ezProcess);
    EZ_SUCCEED_OR_RETURN(m_pOffscreenProcess->Launch(opt));

    m_bExiting = false;
    m_uiReceivedTextures = 0;
    m_pChannel = ezIpcChannel::CreatePipeChannel(sIPC, ezIpcChannel::Mode::Server);
    m_pProtocol = EZ_DEFAULT_NEW(ezIpcProcessMessageProtocol, m_pChannel.Borrow());
    m_pProtocol->m_MessageEvent.AddEventHandler(ezMakeDelegate(&ezRendererTestAdvancedFeatures::OffscreenProcessMessageFunc, this));
    m_pChannel->Connect();
    while (m_pChannel->GetConnectionState() != ezIpcChannel::ConnectionState::Connecting)
    {
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(16));
    }
    m_SharedTextureDesc.SetAsRenderTarget(8, 8, ezGALResourceFormat::BGRAUByteNormalizedsRGB);
    m_SharedTextureDesc.m_Type = ezGALTextureType::Texture2DShared;

    m_SharedTextureQueue.Clear();
    for (ezUInt32 i = 0; i < s_SharedTextureCount; i++)
    {
      m_hSharedTextures[i] = m_pDevice->CreateSharedTexture(m_SharedTextureDesc);
      EZ_TEST_BOOL(!m_hSharedTextures[i].IsInvalidated());
      m_SharedTextureQueue.PushBack({i, 0});
    }

    while (!m_pChannel->IsConnected())
    {
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(16));
      if (m_pOffscreenProcess->GetState() == ezProcessState::Finished)
      {
        ezUInt32 uiExitCode = m_pOffscreenProcess->GetExitCode();
        ezLog::Error("Process exited prematurely with code: {}", uiExitCode);
        return EZ_FAILURE;
      }
    }

    ezOffscreenTest_OpenMsg msg;
    msg.m_TextureDesc = m_SharedTextureDesc;
    for (auto& hSharedTexture : m_hSharedTextures)
    {
      const ezGALSharedTexture* pSharedTexture = m_pDevice->GetSharedTexture(hSharedTexture);
      if (pSharedTexture == nullptr)
      {
        return EZ_FAILURE;
      }

      msg.m_TextureHandles.PushBack(pSharedTexture->GetSharedHandle());
    }
    m_pProtocol->Send(&msg);
  }
#endif

  if (iIdentifier == ST_Tessellation)
  {
    {
      ezGeometry geom;
      geom.AddStackedSphere(0.5f, 3, 2);

      ezMeshBufferResourceDescriptor desc;
      desc.AddStream(ezGALVertexAttributeSemantic::Position, ezGALResourceFormat::XYZFloat);
      desc.AddStream(ezGALVertexAttributeSemantic::TexCoord0, ezGALResourceFormat::XYFloat);
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      m_hSphereMesh = ezResourceManager::CreateResource<ezMeshBufferResource>("UnitTest-SphereMesh", std::move(desc), "SphereMesh");
    }

    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Tessellation.ezShader");
  }

  switch (iIdentifier)
  {
    case SubTests::ST_ReadRenderTarget:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_VertexShaderRenderTargetArrayIndex:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_SharedTexture:
      m_ImgCompFrames.PushBack(100000000);
      break;
    case SubTests::ST_Tessellation:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_Compute:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_FloatSampling:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_ProxyTexture:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      break;
    case SubTests::ST_Material:
      m_ImgCompFrames.PushBack(ImageCaptureFrames::DefaultCapture);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::Material_ColorChange);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::Material_ColorChange2);
      m_ImgCompFrames.PushBack(ImageCaptureFrames::Material_ChangeTexture);
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  return EZ_SUCCESS;
}

ezResult ezRendererTestAdvancedFeatures::DeInitializeSubTest(ezInt32 iIdentifier)
{
  if (iIdentifier == ST_Tessellation)
  {
    m_hSphereMesh.Invalidate();
  }
#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
  else if (iIdentifier == ST_SharedTexture)
  {
    EZ_TEST_BOOL(m_pOffscreenProcess->WaitToFinish(ezTime::MakeFromSeconds(5)).Succeeded());
    EZ_TEST_BOOL(m_pOffscreenProcess->GetState() == ezProcessState::Finished);
    EZ_TEST_INT(m_pOffscreenProcess->GetExitCode(), 0);
    m_pOffscreenProcess = nullptr;

    m_pProtocol = nullptr;
    m_pChannel = nullptr;

    for (ezUInt32 i = 0; i < s_SharedTextureCount; i++)
    {
      m_pDevice->DestroySharedTexture(m_hSharedTextures[i]);
      m_hSharedTextures[i].Invalidate();
    }
    m_SharedTextureQueue.Clear();

    ezStringView sPath = ":imgout/Profiling/sharedTexture.json"_ezsv;
    EZ_TEST_RESULT(ezProfilingUtils::SaveProfilingCapture(sPath));
    ezStringView sPath2 = ":imgout/Profiling/offscreenProfiling.json"_ezsv;
    ezStringView sMergedFile = ":imgout/Profiling/sharedTexturesMerged.json"_ezsv;
    EZ_TEST_RESULT(ezProfilingUtils::MergeProfilingCaptures(sPath, sPath2, sMergedFile));

    ezCVarFloat* pProfilingThreshold = (ezCVarFloat*)ezCVar::FindCVarByName("Profiling.DiscardThresholdMS");
    EZ_ASSERT_DEBUG(pProfilingThreshold, "Profiling.cpp cvar was renamed");
    *pProfilingThreshold = m_fOldProfilingThreshold;
  }
#endif
  else if (iIdentifier == ST_FloatSampling)
  {
    if (!m_hDepthSamplerState.IsInvalidated())
    {
      ezGALDevice::GetDefaultDevice()->DestroySamplerState(m_hDepthSamplerState);
      m_hDepthSamplerState.Invalidate();
    }
  }
  if (iIdentifier == ST_ProxyTexture)
  {
    for (ezUInt32 i = 0; i < 2; i++)
    {
      if (!m_hProxyTexture2D[i].IsInvalidated())
      {
        ezGALDevice::GetDefaultDevice()->DestroyProxyTexture(m_hProxyTexture2D[i]);
        m_hProxyTexture2D[i].Invalidate();
      }
    }
  }
  m_hShader2.Invalidate();
  m_hShader3.Invalidate();

  m_hTexture.Invalidate();
  m_hTexture2.Invalidate();
  m_hMaterial.Invalidate();

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

  DestroyWindow();
  EZ_SUCCEED_OR_RETURN(ezGraphicsTest::DeInitializeSubTest(iIdentifier));
  return EZ_SUCCESS;
}

ezTestAppRun ezRendererTestAdvancedFeatures::RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount)
{
  m_iFrame = uiInvocationCount;
  m_bCaptureImage = false;

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
  if (iIdentifier == ST_SharedTexture)
  {
    return SharedTexture();
  }
#endif
  if (iIdentifier == ST_Material)
  {
    return Material();
  }

  BeginFrame();

  switch (iIdentifier)
  {
    case SubTests::ST_ReadRenderTarget:
      ReadRenderTarget();
      break;
    case SubTests::ST_VertexShaderRenderTargetArrayIndex:
      if (!m_pDevice->GetCapabilities().m_bSupportsVSRenderTargetArrayIndex)
        return ezTestAppRun::Quit;
      VertexShaderRenderTargetArrayIndex();
      break;
    case SubTests::ST_Tessellation:
      Tessellation();
      break;
    case SubTests::ST_Compute:
      Compute();
      break;
    case SubTests::ST_FloatSampling:
      FloatSampling();
      break;
    case SubTests::ST_ProxyTexture:
      ProxyTexture();
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

void ezRendererTestAdvancedFeatures::ReadRenderTarget()
{
  BeginCommands("Offscreen");
  {
    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2D));
    renderingSetup.SetClearColor(0, ezColor::RebeccaPurple);

    ezRectFloat viewport = ezRectFloat(0, 0, 8, 8);
    ezRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    ezRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    ezRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();


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
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D, m_Texture2DRange);
    viewport = ezRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D, m_Texture2DRange);
    viewport = ezRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D, m_Texture2DRange);
    m_bCaptureImage = true;
    viewport = ezRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2D, m_Texture2DRange);
  }
  EndCommands();
}

void ezRendererTestAdvancedFeatures::FloatSampling()
{
  BeginCommands("Offscreen");
  {
    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetDepthStencilTarget(m_pDevice->GetDefaultRenderTargetView(m_hTexture2DArray));
    renderingSetup.SetClearDepth();

    ezRectFloat viewport = ezRectFloat(0, 0, 8, 8);
    ezRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    ezRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    ezRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();


  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezUInt32 uiColumns = 2;
  const ezUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const ezMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  BeginCommands("FloatSampling");
  {
    ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();
    bindGroup.BindSampler("DepthSampler", m_hDepthSamplerState);
    bindGroup.BindTexture("DepthTexture", m_hTexture2DArray);

    ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
    {
      ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0xFFFFFFFF, &viewport);
      RenderObject(m_hCubeUV, mMVP, ezColor(1, 1, 1, 1), ezShaderBindFlags::None);
      EndRendering();
      if (m_ImgCompFrames.Contains(m_iFrame))
      {
        EZ_TEST_IMAGE(m_iFrame, 100);
      }
    }
  }
  EndCommands();
}


void ezRendererTestAdvancedFeatures::ProxyTexture()
{
  // We render normal pattern to layer 0 and the blue pattern to layer 1.
  BeginCommands("Offscreen");
  for (ezUInt32 i = 0; i < 2; i++)
  {
    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hProxyTexture2D[i]));
    renderingSetup.SetClearColor(0, ezColor::RebeccaPurple);

    ezRectFloat viewport = ezRectFloat(0, 0, 8, 8);
    ezRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    ezRenderContext::GetDefaultInstance()->BindShader(i == 0 ? m_hShader2 : m_hShader3);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    ezRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();

  // Render both layers using proxy texture (2D) and manually created resource view (2DArray).
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezUInt32 uiColumns = 2;
  const ezUInt32 uiRows = 2;
  const float fElementWidth = fWidth / uiColumns;
  const float fElementHeight = fHeight / uiRows;

  const ezMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
  BeginCommands("Texture2DProxy");
  {
    ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hProxyTexture2D[0]);
    viewport = ezRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hProxyTexture2D[1]);
    viewport = ezRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray, {0, 1, 0, 1});
    m_bCaptureImage = true;
    viewport = ezRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DArray, {1, 1, 0, 1});
  }
  EndCommands();
}

void ezRendererTestAdvancedFeatures::VertexShaderRenderTargetArrayIndex()
{
  m_bCaptureImage = true;
  const ezMat4 mMVP = CreateSimpleMVP((m_pWindow->GetClientAreaSize().width / 2.0f) / (float)m_pWindow->GetClientAreaSize().height);
  BeginCommands("Offscreen Stereo");
  {
    ezGALRenderingSetup renderingSetup;
    renderingSetup.SetColorTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2DArray));
    renderingSetup.SetClearColor(0, ezColor::RebeccaPurple);

    ezRectFloat viewport = ezRectFloat(0, 0, m_pWindow->GetClientAreaSize().width / 2.0f, (float)m_pWindow->GetClientAreaSize().height);
    ezRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    ezRenderContext::GetDefaultInstance()->BindShader(m_hShader, ezShaderBindFlags::None);
    ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
    ocb->m_MVP = mMVP;
    ocb->m_Color = ezColor(1, 1, 1, 1);
    ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();
    bindGroup.BindBuffer("PerObject", m_hObjectTransformCB);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hCubeUV);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer(0xFFFFFFFF, 0, 2).IgnoreResult();

    ezRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();


  BeginCommands("Texture2DArray");
  {
    ezRectFloat viewport = ezRectFloat(0, 0, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);

    ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0xFFFFFFFF, &viewport);

    ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();
    bindGroup.BindTexture("DiffuseTexture", m_hTexture2DArray);

    ezRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    EndRendering();
    if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
    {
      EZ_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}

void ezRendererTestAdvancedFeatures::Tessellation()
{
  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
  const ezMat4 mMVP = CreateSimpleMVP((float)fWidth / (float)fHeight);
  BeginCommands("Tessellation");
  {
    ezRectFloat viewport = ezRectFloat(0, 0, fWidth, fHeight);
    ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0xFFFFFFFF, &viewport);
    RenderObject(m_hSphereMesh, mMVP, ezColor(1, 1, 1, 1), ezShaderBindFlags::None);

    EndRendering();
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      EZ_TEST_IMAGE(m_iFrame, 100);
    }
  }
  EndCommands();
}


void ezRendererTestAdvancedFeatures::Compute()
{
  BeginCommands("Compute");
  {
    ezUInt32 uiWidth = 8;
    ezUInt32 uiHeight = 8;

    ezRenderContext::GetDefaultInstance()->BeginCompute("Compute");
    {
      ezRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
      ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();

      ezGALTextureRange textureRange;
      textureRange.m_uiBaseMipLevel = 4;
      textureRange.m_uiMipLevels = 1;
      textureRange.m_uiBaseArraySlice = 0;
      textureRange.m_uiArraySlices = 2;

      bindGroup.BindTexture("OutputTexture", m_hTexture2D, textureRange);

      // The compute shader uses [numthreads(8, 8, 1)], so we need to compute how many of these groups we need to dispatch to fill the entire image.
      constexpr ezUInt32 uiThreadsX = 8;
      constexpr ezUInt32 uiThreadsY = 8;
      const ezUInt32 uiDispatchX = (uiWidth + uiThreadsX - 1) / uiThreadsX;
      const ezUInt32 uiDispatchY = (uiHeight + uiThreadsY - 1) / uiThreadsY;
      // As the image is exactly as big as one of our groups, we need to dispatch exactly one group:
      EZ_TEST_INT(uiDispatchX, 1);
      EZ_TEST_INT(uiDispatchY, 1);
      ezRenderContext::GetDefaultInstance()->Dispatch(uiDispatchX, uiDispatchY, 2).AssertSuccess();
    }
    ezRenderContext::GetDefaultInstance()->EndCompute();
  }
  EndCommands();


  const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
  const float fHeight = (float)m_pWindow->GetClientAreaSize().height;

  const ezMat4 mMVP = CreateSimpleMVP((float)fWidth / (float)fHeight);
  BeginCommands("Texture2D");
  {
    m_bCaptureImage = true;
    ezRectFloat viewport = ezRectFloat(0, 0, fWidth, fHeight);
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2D, m_Texture2DRange);
  }
  EndCommands();
}

ezTestAppRun ezRendererTestAdvancedFeatures::Material()
{
  {
    ezResourceLock<ezMaterialResource> pMaterial(m_hMaterial, ezResourceAcquireMode::BlockTillLoaded);
    const ezMaterialResourceDescriptor& desc = pMaterial->GetCurrentDesc();
    EZ_TEST_INT(desc.m_PermutationVars.GetCount(), 0);
    EZ_TEST_INT(desc.m_Parameters.GetCount(), 2);
    EZ_TEST_INT(desc.m_Texture2DBindings.GetCount(), 1);
    EZ_TEST_INT(desc.m_TextureCubeBindings.GetCount(), 0);
    ezVariant color1 = pMaterial->GetParameter(m_sBaseColor);
    ezVariant color2 = pMaterial->GetParameter(m_sBaseColor2);
    ezTexture2DResourceHandle hTexture = pMaterial->GetTexture2DBinding(m_sTexture);

    if (m_iFrame == ImageCaptureFrames::Material_ColorChange)
    {
      EZ_TEST_BOOL(color1.IsA<ezColor>() && color1.Get<ezColor>() == ezColor::White);
      EZ_TEST_BOOL(color2.IsA<ezColor>() && color2.Get<ezColor>() == ezColor::White);
      EZ_TEST_BOOL(hTexture == m_hTexture);
      pMaterial->SetParameter(m_sBaseColor, ezColor::Yellow);
    }
    else if (m_iFrame == ImageCaptureFrames::Material_ColorChange2)
    {
      EZ_TEST_BOOL(color1.IsA<ezColor>() && color1.Get<ezColor>() == ezColor::Yellow);
      EZ_TEST_BOOL(color2.IsA<ezColor>() && color2.Get<ezColor>() == ezColor::White);
      EZ_TEST_BOOL(hTexture == m_hTexture);
      pMaterial->SetParameter(m_sBaseColor2, ezColor::Cyan);
    }
    else if (m_iFrame == ImageCaptureFrames::Material_ChangeTexture)
    {
      EZ_TEST_BOOL(color1.IsA<ezColor>() && color1.Get<ezColor>() == ezColor::Yellow);
      EZ_TEST_BOOL(color2.IsA<ezColor>() && color2.Get<ezColor>() == ezColor::Cyan);
      EZ_TEST_BOOL(hTexture == m_hTexture);
      pMaterial->SetTexture2DBinding(m_sTexture, m_hTexture2);
    }
  }

  BeginFrame();
  {
    const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
    const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
    const ezMat4 mMVP = CreateSimpleMVP((float)fWidth / (float)fHeight);
    BeginCommands("Tessellation");
    {
      ezRectFloat viewport = ezRectFloat(0, 0, fWidth, fHeight);
      ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0xFFFFFFFF, &viewport);

      ezRenderContext* pContext = ezRenderContext::GetDefaultInstance();
      pContext->BindMaterial(m_hMaterial);

      ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
      ocb->m_MVP = mMVP;
      ocb->m_Color = ezColor(1, 1, 1, 1);

      ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();
      bindGroup.BindBuffer("PerObject", m_hObjectTransformCB);

      ezRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hCubeUV);
      ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().IgnoreResult();

      EndRendering();
      if (m_ImgCompFrames.Contains(m_iFrame))
      {
        EZ_TEST_IMAGE(m_iFrame, 100);
      }
    }
    EndCommands();
  }
  EndFrame();

  if (m_ImgCompFrames.IsEmpty() || m_ImgCompFrames.PeekBack() == m_iFrame)
  {
    return ezTestAppRun::Quit;
  }
  return ezTestAppRun::Continue;
}

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
ezTestAppRun ezRendererTestAdvancedFeatures::SharedTexture()
{
  if (m_pOffscreenProcess->GetState() != ezProcessState::Running)
  {
    EZ_TEST_BOOL(m_bExiting);
    return ezTestAppRun::Quit;
  }

  m_pProtocol->WaitForMessages(ezTime::MakeFromMilliseconds(16)).IgnoreResult();

  ezOffscreenTest_SharedTexture texture = m_SharedTextureQueue.PeekFront();
  m_SharedTextureQueue.PopFront();

  ezStringBuilder sTemp;
  sTemp.SetFormat("Render {}:{}|{}", m_uiReceivedTextures, texture.m_uiCurrentTextureIndex, texture.m_uiCurrentSemaphoreValue);
  EZ_PROFILE_SCOPE(sTemp);
  BeginFrame();
  {
    const ezGALSharedTexture* pSharedTexture = m_pDevice->GetSharedTexture(m_hSharedTextures[texture.m_uiCurrentTextureIndex]);
    EZ_ASSERT_DEV(pSharedTexture != nullptr, "Shared texture did not resolve");

    pSharedTexture->WaitSemaphoreGPU(texture.m_uiCurrentSemaphoreValue);

    const float fWidth = (float)m_pWindow->GetClientAreaSize().width;
    const float fHeight = (float)m_pWindow->GetClientAreaSize().height;
    const ezUInt32 uiColumns = 1;
    const ezUInt32 uiRows = 1;
    const float fElementWidth = fWidth / uiColumns;
    const float fElementHeight = fHeight / uiRows;

    const ezMat4 mMVP = CreateSimpleMVP((float)fElementWidth / (float)fElementHeight);
    BeginCommands("Texture2D");
    {
      ezRectFloat viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);
      m_bCaptureImage = true;
      viewport = ezRectFloat(0, 0, fElementWidth, fElementHeight);

      ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0xFFFFFFFF, &viewport);

      ezBindGroupBuilder& bindGroup = ezRenderContext::GetDefaultInstance()->GetBindGroup();
      bindGroup.BindTexture("DiffuseTexture", m_hSharedTextures[texture.m_uiCurrentTextureIndex]);
      RenderObject(m_hCubeUV, mMVP, ezColor(1, 1, 1, 1), ezShaderBindFlags::None);

      EndRendering();
      if (!m_bExiting && m_uiReceivedTextures > 10)
      {
        EZ_TEST_IMAGE(0, 10);

        ezOffscreenTest_CloseMsg msg;
        EZ_TEST_BOOL(m_pProtocol->Send(&msg));
        m_bExiting = true;
      }
    }
    EndCommands();

    texture.m_uiCurrentSemaphoreValue++;
    pSharedTexture->SignalSemaphoreGPU(texture.m_uiCurrentSemaphoreValue);
  }
  EndFrame();

  if (m_SharedTextureQueue.IsEmpty() || !m_pChannel->IsConnected())
  {
    m_SharedTextureQueue.PushBack(texture);
  }
  else if (!m_bExiting)
  {
    ezOffscreenTest_RenderMsg msg;
    msg.m_Texture = texture;
    EZ_TEST_BOOL(m_pProtocol->Send(&msg));
  }

  return ezTestAppRun::Continue;
}

void ezRendererTestAdvancedFeatures::OffscreenProcessMessageFunc(const ezProcessMessage* pMsg)
{
  if (const auto* pAction = ezDynamicCast<const ezOffscreenTest_RenderResponseMsg*>(pMsg))
  {
    m_uiReceivedTextures++;
    ezStringBuilder sTemp;
    sTemp.SetFormat("Receive {}|{}", pAction->m_Texture.m_uiCurrentTextureIndex, pAction->m_Texture.m_uiCurrentSemaphoreValue);
    EZ_PROFILE_SCOPE(sTemp);
    m_SharedTextureQueue.PushBack(pAction->m_Texture);
  }
}
#endif

static ezRendererTestAdvancedFeatures g_AdvancedFeaturesTest;
