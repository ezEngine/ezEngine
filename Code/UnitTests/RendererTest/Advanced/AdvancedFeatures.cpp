#include <RendererTest/RendererTestPCH.h>

#include <Core/GameState/GameStateWindow.h>
#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <RendererTest/Advanced/AdvancedFeatures.h>

#undef CreateWindow


void ezRendererTestAdvancedFeatures::SetupSubTests()
{
  ezStartup::StartupCoreSystems();
  SetupRenderer().AssertSuccess();
  const ezGALDeviceCapabilities& caps = ezGALDevice::GetDefaultDevice()->GetCapabilities();

  AddSubTest("01 - ReadRenderTarget", SubTests::ST_ReadRenderTarget);
  if (caps.m_bVertexShaderRenderTargetArrayIndex)
  {
    AddSubTest("02 - VertexShaderRenderTargetArrayIndex", SubTests::ST_VertexShaderRenderTargetArrayIndex);
  }
#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
  if (caps.m_bSharedTextures)
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

  ShutdownRenderer();
  ezStartup::ShutdownCoreSystems();
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

    ezGALTextureResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture = m_hTexture2D;
    viewDesc.m_uiMipLevelsToUse = 1;
    for (ezUInt32 i = 0; i < 4; i++)
    {
      viewDesc.m_uiMostDetailedMipLevel = 0;
      m_hTexture2DMips[i] = m_pDevice->CreateResourceView(viewDesc);
    }

    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/UVColor.ezShader");
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2D.ezShader");
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

    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(8, 8, textureFormat, ezGALMSAASampleCount::None);
    desc.m_bAllowUAV = true;
    desc.m_bCreateRenderTarget = false;
    desc.m_uiMipLevelCount = 1;
    desc.m_ResourceAccess.m_bImmutable = false;
    m_hTexture2D = m_pDevice->CreateTexture(desc);

    ezGALTextureResourceViewCreationDescription viewDesc;
    viewDesc.m_hTexture = m_hTexture2D;
    viewDesc.m_uiMipLevelsToUse = 1;
    viewDesc.m_uiMostDetailedMipLevel = 0;
    m_hTexture2DMips[0] = m_pDevice->CreateResourceView(viewDesc);

    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/UVColorCompute.ezShader");
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Texture2D.ezShader");
  }

  if (iIdentifier == ST_VertexShaderRenderTargetArrayIndex)
  {
    // Texture2DArray
    ezGALTextureCreationDescription desc;
    desc.SetAsRenderTarget(320 / 2, 240, ezGALResourceFormat::BGRAUByteNormalizedsRGB, ezGALMSAASampleCount::None);
    desc.m_uiArraySize = 2;
    m_hTexture2DArray = m_pDevice->CreateTexture(desc);

    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/Stereo.ezShader");
    m_hShader2 = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/StereoPreview.ezShader");
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
  m_hShader2.Invalidate();

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

  for (ezUInt32 i = 0; i < 4; i++)
  {
    m_hTexture2DMips[i].Invalidate();
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

  BeginFrame();

  switch (iIdentifier)
  {
    case SubTests::ST_ReadRenderTarget:
      ReadRenderTarget();
      break;
    case SubTests::ST_VertexShaderRenderTargetArrayIndex:
      if (!m_pDevice->GetCapabilities().m_bVertexShaderRenderTargetArrayIndex)
        return ezTestAppRun::Quit;
      VertexShaderRenderTargetArrayIndex();
      break;
    case SubTests::ST_Tessellation:
      Tessellation();
      break;
    case SubTests::ST_Compute:
      Compute();
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
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2D));
    renderingSetup.m_ClearColor = ezColor::RebeccaPurple;
    renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;

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
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2DMips[0]);
    viewport = ezRectFloat(fElementWidth, 0, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DMips[0]);
    viewport = ezRectFloat(0, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DMips[0]);
    m_bCaptureImage = true;
    viewport = ezRectFloat(fElementWidth, fElementHeight, fElementWidth, fElementHeight);
    RenderCube(viewport, mMVP, 0, m_hTexture2DMips[0]);
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
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, m_pDevice->GetDefaultRenderTargetView(m_hTexture2DArray));
    renderingSetup.m_ClearColor = ezColor::RebeccaPurple;
    renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;

    ezRectFloat viewport = ezRectFloat(0, 0, m_pWindow->GetClientAreaSize().width / 2.0f, (float)m_pWindow->GetClientAreaSize().height);
    ezRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    SetClipSpace();

    ezRenderContext::GetDefaultInstance()->BindShader(m_hShader, ezShaderBindFlags::None);
    ObjectCB* ocb = ezRenderContext::GetConstantBufferData<ObjectCB>(m_hObjectTransformCB);
    ocb->m_MVP = mMVP;
    ocb->m_Color = ezColor(1, 1, 1, 1);
    ezRenderContext::GetDefaultInstance()->BindConstantBuffer("PerObject", m_hObjectTransformCB);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(m_hCubeUV);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer(0xFFFFFFFF, 0, 2).IgnoreResult();

    ezRenderContext::GetDefaultInstance()->EndRendering();
  }
  EndCommands();


  BeginCommands("Texture2DArray");
  {
    ezRectFloat viewport = ezRectFloat(0, 0, (float)m_pWindow->GetClientAreaSize().width, (float)m_pWindow->GetClientAreaSize().height);

    ezGALCommandEncoder* pCommandEncoder = BeginRendering(ezColor::RebeccaPurple, 0xFFFFFFFF, &viewport);

    ezRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", m_pDevice->GetDefaultResourceView(m_hTexture2DArray));

    ezRenderContext::GetDefaultInstance()->BindShader(m_hShader2);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    if (m_bCaptureImage && m_ImgCompFrames.Contains(m_iFrame))
    {
      EZ_TEST_IMAGE(m_iFrame, 100);
    }

    EndRendering();
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
    if (m_ImgCompFrames.Contains(m_iFrame))
    {
      EZ_TEST_IMAGE(m_iFrame, 100);
    }
    EndRendering();
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

      ezGALTextureUnorderedAccessViewHandle hFilterOutput;
      {
        ezGALTextureUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = m_hTexture2D;
        desc.m_uiMipLevelToUse = 0;
        desc.m_uiFirstArraySlice = 0;
        desc.m_uiArraySize = 1;
        hFilterOutput = m_pDevice->CreateUnorderedAccessView(desc);
      }
      ezRenderContext::GetDefaultInstance()->BindUAV("OutputTexture", hFilterOutput);

      // The compute shader uses [numthreads(8, 8, 1)], so we need to compute how many of these groups we need to dispatch to fill the entire image.
      constexpr ezUInt32 uiThreadsX = 8;
      constexpr ezUInt32 uiThreadsY = 8;
      const ezUInt32 uiDispatchX = (uiWidth + uiThreadsX - 1) / uiThreadsX;
      const ezUInt32 uiDispatchY = (uiHeight + uiThreadsY - 1) / uiThreadsY;
      // As the image is exactly as big as one of our groups, we need to dispatch exactly one group:
      EZ_TEST_INT(uiDispatchX, 1);
      EZ_TEST_INT(uiDispatchY, 1);
      ezRenderContext::GetDefaultInstance()->Dispatch(uiDispatchX, uiDispatchY, 1).AssertSuccess();
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
    RenderCube(viewport, mMVP, 0xFFFFFFFF, m_hTexture2DMips[0]);
  }
  EndCommands();
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

      ezRenderContext::GetDefaultInstance()->BindTexture2D("DiffuseTexture", m_pDevice->GetDefaultResourceView(m_hSharedTextures[texture.m_uiCurrentTextureIndex]));
      RenderObject(m_hCubeUV, mMVP, ezColor(1, 1, 1, 1), ezShaderBindFlags::None);


      if (!m_bExiting && m_uiReceivedTextures > 10)
      {
        EZ_TEST_IMAGE(0, 10);

        ezOffscreenTest_CloseMsg msg;
        EZ_TEST_BOOL(m_pProtocol->Send(&msg));
        m_bExiting = true;
      }
      EndRendering();
    }
    EndCommands();

    texture.m_uiCurrentSemaphoreValue++;
    pSharedTexture->SignalSemaphoreGPU(texture.m_uiCurrentSemaphoreValue);
    ezRenderContext::GetDefaultInstance()->ResetContextState();
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
