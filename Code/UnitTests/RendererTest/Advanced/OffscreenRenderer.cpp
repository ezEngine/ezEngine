#include <RendererTest/RendererTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/ProfilingUtils.h>
#include <Foundation/Time/Clock.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SharedTextureSwapChain.h>
#include <RendererTest/Advanced/OffscreenRenderer.h>
#include <RendererTest/TestClass/TestClass.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezOffscreenTest_SharedTexture, ezNoBase, 1, ezRTTIDefaultAllocator<ezOffscreenTest_SharedTexture>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CurrentTextureIndex", m_uiCurrentTextureIndex),
    EZ_MEMBER_PROPERTY("CurrentSemaphoreValue", m_uiCurrentSemaphoreValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezOffscreenTest_OpenMsg, 1, ezRTTIDefaultAllocator<ezOffscreenTest_OpenMsg>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("TextureDesc", m_TextureDesc),
      EZ_ARRAY_MEMBER_PROPERTY("TextureHandles", m_TextureHandles),
    }
    EZ_END_PROPERTIES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezOffscreenTest_CloseMsg, 1, ezRTTIDefaultAllocator<ezOffscreenTest_CloseMsg>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezOffscreenTest_RenderMsg, 1, ezRTTIDefaultAllocator<ezOffscreenTest_RenderMsg>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("Texture", m_Texture),
    }
    EZ_END_PROPERTIES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezOffscreenTest_RenderResponseMsg, 1, ezRTTIDefaultAllocator<ezOffscreenTest_RenderResponseMsg>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("Texture", m_Texture),
    }
    EZ_END_PROPERTIES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezOffscreenRendererTest::ezOffscreenRendererTest()
  : ezApplication("ezOffscreenRendererTest")

{
}

ezOffscreenRendererTest::~ezOffscreenRendererTest() = default;

void ezOffscreenRendererTest::Run()
{
  EZ_PROFILE_SCOPE("Run");

  ezClock::GetGlobalClock()->Update();

  if (!m_pProtocol->ProcessMessages())
  {
    m_pProtocol->WaitForMessages(ezTime::MakeFromMilliseconds(8)).IgnoreResult();
  }

  // do the rendering
  if (!m_RequestedFrames.IsEmpty())
  {
    ezOffscreenTest_RenderMsg action = m_RequestedFrames[0];
    m_RequestedFrames.RemoveAtAndCopy(0);

    auto device = ezGALDevice::GetDefaultDevice();

    auto pSwapChain = const_cast<ezGALSharedTextureSwapChain*>(ezGALDevice::GetDefaultDevice()->GetSwapChain<ezGALSharedTextureSwapChain>(m_hSwapChain));
    EZ_ASSERT_DEBUG(pSwapChain, "SwapChain should have been created at this point");
    EZ_ANALYSIS_ASSUME(pSwapChain != nullptr);
    pSwapChain->Arm(action.m_Texture.m_uiCurrentTextureIndex, action.m_Texture.m_uiCurrentSemaphoreValue);

    ezStringBuilder sTemp;
    sTemp.SetFormat("Render {}|{}", action.m_Texture.m_uiCurrentTextureIndex, action.m_Texture.m_uiCurrentSemaphoreValue);
    EZ_PROFILE_SCOPE(sTemp);

    m_pDevice->EnqueueFrameSwapChain(m_hSwapChain);
    device->BeginFrame();

    ezGALCommandEncoder* pCommandEncoder = device->BeginCommands(sTemp);

    ezGALRenderingSetup renderingSetup;
    ezGALRenderTargetViewHandle hBackbufferRTV = device->GetDefaultRenderTargetView(pSwapChain->GetRenderTargets().m_hRTs[0]);
    renderingSetup.SetColorTarget(0, hBackbufferRTV);
    renderingSetup.SetClearColor(0, ezColor::Pink);

    ezRectFloat viewport = ezRectFloat(0, 0, 8, 8);
    ezRenderContext::GetDefaultInstance()->BeginRendering(renderingSetup, viewport);
    ezGraphicsTest::SetClipSpace();

    ezRenderContext::GetDefaultInstance()->BindShader(m_hScreenShader);
    ezRenderContext::GetDefaultInstance()->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
    ezRenderContext::GetDefaultInstance()->DrawMeshBuffer().AssertSuccess();

    ezRenderContext::GetDefaultInstance()->EndRendering();

    device->EndCommands(pCommandEncoder);

    device->EndFrame();
  }

  if (m_RequestedFrames.IsEmpty() && m_bExiting)
  {
    SetReturnCode(0);
    RequestApplicationQuit();
  }

  // needs to be called once per frame
  ezResourceManager::PerFrameUpdate();

  // tell the task system to finish its work for this frame
  // this has to be done at the very end, so that the task system will only use up the time that is left in this frame for
  // uploading GPU data etc.
  ezTaskSystem::FinishFrameTasks();
}

void ezOffscreenRendererTest::OnPresent(ezUInt32 uiCurrentTexture, ezUInt64 uiCurrentSemaphoreValue)
{
  ezStringBuilder sTemp;
  sTemp.SetFormat("Response {}|{}", uiCurrentTexture, uiCurrentSemaphoreValue);
  EZ_PROFILE_SCOPE(sTemp);

  ezOffscreenTest_RenderResponseMsg msg = {};
  msg.m_Texture.m_uiCurrentSemaphoreValue = uiCurrentSemaphoreValue;
  msg.m_Texture.m_uiCurrentTextureIndex = uiCurrentTexture;
  m_pProtocol->Send(&msg);
}

void ezOffscreenRendererTest::AfterCoreSystemsStartup()
{
  SUPER::AfterCoreSystemsStartup();

  ezGraphicsTest::CreateRenderer(m_pDevice).AssertSuccess();

  ezGlobalLog::AddLogWriter(ezLoggingEvent::Handler(&ezLogWriter::HTML::LogMessageHandler, &m_LogHTML));
  ezStringBuilder sLogFile;
  sLogFile.SetFormat(":imgout/OffscreenLog.htm");
  m_LogHTML.BeginLog(sLogFile, "OffscreenRenderer"_ezsv);

  // Setup Shaders and Materials
  {
    m_hScreenShader = ezResourceManager::LoadResource<ezShaderResource>("RendererTest/Shaders/UVColor.ezShader");
  }

  if (ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-IPC").IsEmpty())
  {
    EZ_REPORT_FAILURE("Command Line does not contain -IPC parameter");
    SetReturnCode(-1);
    RequestApplicationQuit();
    return;
  }

  if (ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-PID").IsEmpty())
  {
    EZ_REPORT_FAILURE("Command Line does not contain -PID parameter");
    SetReturnCode(-2);
    RequestApplicationQuit();
    return;
  }

  m_iHostPID = 0;
  if (ezConversionUtils::StringToInt64(ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-PID"), m_iHostPID).Failed())
  {
    EZ_REPORT_FAILURE("Command Line -PID parameter could not be converted to int");
    SetReturnCode(-3);
    RequestApplicationQuit();
    return;
  }

  ezLog::Debug("Host Process ID: {0}", m_iHostPID);

  m_pChannel = ezIpcChannel::CreatePipeChannel(ezCommandLineUtils::GetGlobalInstance()->GetStringOption("-IPC"), ezIpcChannel::Mode::Client);
  m_pProtocol = EZ_DEFAULT_NEW(ezIpcProcessMessageProtocol, m_pChannel.Borrow());
  m_pProtocol->m_MessageEvent.AddEventHandler(ezMakeDelegate(&ezOffscreenRendererTest::MessageFunc, this));
  m_pChannel->Connect();

  while (!m_pChannel->IsConnected())
  {
    ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(16));
  }

  ezStartup::StartupHighLevelSystems();
}

void ezOffscreenRendererTest::BeforeHighLevelSystemsShutdown()
{
  ezStringView sPath = ":imgout/Profiling/offscreenProfiling.json"_ezsv;
  EZ_TEST_RESULT(ezProfilingUtils::SaveProfilingCapture(sPath));

  auto pDevice = ezGALDevice::GetDefaultDevice();

  pDevice->DestroySwapChain(m_hSwapChain);
  // This guarantees that when the process exits no shared textures are still being modified by the GPU so the main process can savely delete the resources.
  pDevice->WaitIdle();

  m_hSwapChain.Invalidate();

  m_hScreenShader.Invalidate();

  m_pProtocol = nullptr;
  m_pChannel = nullptr;

  ezGlobalLog::RemoveLogWriter(ezLoggingEvent::Handler(&ezLogWriter::HTML::LogMessageHandler, &m_LogHTML));
  m_LogHTML.EndLog();

  SUPER::BeforeHighLevelSystemsShutdown();
}

void ezOffscreenRendererTest::BeforeCoreSystemsShutdown()
{
  ezResourceManager::FreeAllUnusedResources();

  if (m_pDevice)
  {
    m_pDevice->Shutdown().IgnoreResult();
    EZ_DEFAULT_DELETE(m_pDevice);
  }

  SUPER::BeforeCoreSystemsShutdown();
}

void ezOffscreenRendererTest::MessageFunc(const ezProcessMessage* pMsg)
{
  if (const auto* pAction = ezDynamicCast<const ezOffscreenTest_OpenMsg*>(pMsg))
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    EZ_ASSERT_DEBUG(m_hSwapChain.IsInvalidated(), "SwapChain creation should only happen once");

    ezGALSharedTextureSwapChainCreationDescription desc;
    desc.m_TextureDesc = pAction->m_TextureDesc;
    desc.m_Textures = pAction->m_TextureHandles;
    desc.m_OnPresent = ezMakeDelegate(&ezOffscreenRendererTest::OnPresent, this);

    m_hSwapChain = ezGALSharedTextureSwapChain::Create(desc);
    if (m_hSwapChain.IsInvalidated())
    {
      EZ_REPORT_FAILURE("Failed to create shared texture swapchain");
      SetReturnCode(-4);
      RequestApplicationQuit();
    }
  }
  else if (const auto* pAction = ezDynamicCast<const ezOffscreenTest_CloseMsg*>(pMsg))
  {
    m_bExiting = true;
  }
  else if (const auto* pAction = ezDynamicCast<const ezOffscreenTest_RenderMsg*>(pMsg))
  {
    EZ_ASSERT_DEBUG(m_bExiting == false, "No new frame requests should come in at this point.");
    m_RequestedFrames.PushBack(*pAction);
  }
}
