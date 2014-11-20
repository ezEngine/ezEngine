#include <PCH.h>
#include <Core/Application/Application.h>
#include <EditorFramework/EditorFramework.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererCore/Pipeline/RenderHelper.h>
#include <System/Window/Window.h>
#include <QApplication>
#include <QMessageBox>
#include <EditorFramework/IPC/ProcessCommunication.h>

class ezOtherProcessWindow : public ezWindowBase
{
public:
  ezOtherProcessWindow()
  {
    m_hWnd = 0;
    m_uiWidth = 0;
    m_uiHeight = 0;
  }

  virtual ezSizeU32 GetClientAreaSize() const override { return ezSizeU32(m_uiWidth, m_uiHeight); }
  virtual ezWindowHandle GetNativeWindowHandle() const override { return m_hWnd; }

  ezWindowHandle m_hWnd;
  ezUInt16 m_uiWidth;
  ezUInt16 m_uiHeight;
};

class ezEditorEngineViewApp : public ezApplication
{
  ezProcessCommunication m_IPC;

public:
  ezEditorEngineViewApp()
  {
    EnableMemoryLeakReporting(true);
    m_pApp = nullptr;
  }

  void EventHandlerIPC(const ezProcessCommunication::Event& e)
  {
    const ezEngineViewMsg* pMsg = (const ezEngineViewMsg*) e.m_pMessage;

    ezStringBuilder s;
    s.Format("Msg: '%s', HWND: %u, Size = %u * %u", pMsg->GetDynamicRTTI()->GetTypeName(), pMsg->m_uiHWND, pMsg->m_uiWindowWidth, pMsg->m_uiWindowHeight);
    //QMessageBox::information(nullptr, "Engine View", QLatin1String(s.GetData()), QMessageBox::StandardButton::Ok);

    InitWindow((HWND) pMsg->m_uiHWND, pMsg->m_uiWindowWidth, pMsg->m_uiWindowHeight);
  }

  virtual void AfterEngineInit() override
  {
    //m_sChannel = ezCommandLineUtils::GetInstance()->GetStringOption("-channel", 0);

    int argc = GetArgumentCount();
    const char** argv = GetArgumentsArray();
    m_pApp = new QApplication(argc, (char**) argv);

    EZ_VERIFY(m_IPC.ConnectToHostProcess().Succeeded(), "Could not connect to host");

    m_IPC.m_Events.AddEventHandler(ezDelegate<void (const ezProcessCommunication::Event&)> (&ezEditorEngineViewApp::EventHandlerIPC, this));

    //m_pMemory = new QSharedMemory(m_sChannel.GetData());
    //EZ_VERIFY(m_pMemory->attach(), "No host memory available");

    //EZ_VERIFY(m_pMemory->lock(), "Failed to lock shared memory");

    //ezUInt32 uiWnd = *((ezUInt32*) m_pMemory->data());
    //m_hWnd = (HWND) uiWnd;

    //char szTemp[1024];
    //ezStringUtils::Copy(szTemp, 1024, (const char*) m_pMemory->constData());

    //EZ_VERIFY(m_pMemory->unlock(), "Failed to unlock shared memory");

    //QMessageBox::information(nullptr, m_sChannel.GetData(), szTemp, QMessageBox::StandardButton::Ok);

    //m_OtherWindow.m_hWnd = m_hWnd;

    InitDevice();
  }

  virtual void BeforeEngineShutdown() override
  {
    m_IPC.m_Events.RemoveEventHandler(ezDelegate<void (const ezProcessCommunication::Event&)> (&ezEditorEngineViewApp::EventHandlerIPC, this));

    delete m_pApp;
  }

  virtual ApplicationExecution Run() override
  {
    m_IPC.ProcessMessages();

    if (m_OtherWindow.m_hWnd == 0)
      return ezApplication::Continue;

    //if (m_sChannel.IsEmpty())
    //{
    //  SetReturnCode(1);
    //  return ezApplication::Quit;
    //}

    //App.exec();
    //QMessageBox::information(nullptr, "Success", m_sChannel.GetData(), QMessageBox::StandardButton::Ok);

    {
      // Before starting to render in a frame call this function
      s_pDevice->BeginFrame();

      // The ezGALContext class is the main interaction point for draw / compute operations
      ezGALContext* pContext = s_pDevice->GetPrimaryContext();

      ezSizeU32 wndsize = m_OtherWindow.GetClientAreaSize();

      pContext->SetRenderTargetConfig(m_hBBRT);
      pContext->SetViewport(0.0f, 0.0f, (float) wndsize.width, (float) wndsize.height, 0.0f, 1.0f);

      static float fBlue = 0;
      fBlue = ezMath::Mod(fBlue + 0.01f, 1.0f);
      ezColor c(0.1f, 0.1f, fBlue);
      pContext->Clear(c);

      pContext->SetRasterizerState(m_hRasterizerState);
      pContext->SetDepthStencilState(m_hDepthStencilState);

      s_pDevice->Present(m_hPrimarySwapChain);

      s_pDevice->EndFrame();

    }

    ezThreadUtils::Sleep(1);

    return ezApplication::Continue;
  }

private:

  void InitDevice()
  {
    // Create a device
    ezGALDeviceCreationDescription DeviceInit;
    DeviceInit.m_bCreatePrimarySwapChain = false;
    DeviceInit.m_bDebugDevice = true;

    s_pDevice = EZ_DEFAULT_NEW(ezGALDeviceDX11)(DeviceInit);

    EZ_VERIFY(s_pDevice->Init() == EZ_SUCCESS, "Device init failed!");

    ezGALDevice::SetDefaultDevice(s_pDevice);
  }

  void InitWindow(HWND hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight)
  {
    m_hWnd = hWnd;
    m_OtherWindow.m_hWnd = hWnd;
    m_OtherWindow.m_uiWidth = uiWidth;
    m_OtherWindow.m_uiHeight = uiHeight;

    {
      ezGALSwapChainCreationDescription scd;
      scd.m_pWindow = &m_OtherWindow;
      scd.m_SampleCount = ezGALMSAASampleCount::None;
      scd.m_bCreateDepthStencilBuffer = true;
      scd.m_DepthStencilBufferFormat = ezGALResourceFormat::D24S8;
      scd.m_bAllowScreenshots = true;
      scd.m_bVerticalSynchronization = true;

      m_hPrimarySwapChain = s_pDevice->CreateSwapChain(scd);
      const ezGALSwapChain* pPrimarySwapChain = s_pDevice->GetSwapChain(m_hPrimarySwapChain);
      EZ_ASSERT(pPrimarySwapChain != nullptr, "Failed to init swapchain");

      m_hBBRT = pPrimarySwapChain->GetRenderTargetViewConfig();
      EZ_ASSERT(!m_hBBRT.IsInvalidated(), "Failed to init render target");
    }

    ezGALRasterizerStateCreationDescription RasterStateDesc;
    RasterStateDesc.m_bWireFrame = true;
    RasterStateDesc.m_CullMode = ezGALCullMode::Back;
    RasterStateDesc.m_bFrontCounterClockwise = true;
    m_hRasterizerState = s_pDevice->CreateRasterizerState(RasterStateDesc);
    EZ_ASSERT(!m_hRasterizerState.IsInvalidated(), "Couldn't create rasterizer state!");

    ezGALDepthStencilStateCreationDescription DepthStencilStateDesc;
    DepthStencilStateDesc.m_bDepthTest = true;
    DepthStencilStateDesc.m_bDepthWrite = true;
    m_hDepthStencilState = s_pDevice->CreateDepthStencilState(DepthStencilStateDesc);
    EZ_ASSERT(!m_hDepthStencilState.IsInvalidated(), "Couldn't create depth-stencil state!");
  }

  ezOtherProcessWindow m_OtherWindow;
  //ezString m_sChannel;
  QApplication* m_pApp;
  //QSharedMemory* m_pMemory;
  HWND m_hWnd;
  ezGALDevice* s_pDevice;
  ezGALRenderTargetConfigHandle m_hBBRT;
  ezGALBufferHandle m_hCB;
  ezGALRasterizerStateHandle m_hRasterizerState;
  ezGALDepthStencilStateHandle m_hDepthStencilState;
  ezGALSwapChainHandle m_hPrimarySwapChain;

};

EZ_APPLICATION_ENTRY_POINT(ezEditorEngineViewApp);

