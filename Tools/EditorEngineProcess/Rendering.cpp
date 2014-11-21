#include <PCH.h>
#include <EditorEngineProcess/Application.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererDX11/Device/DeviceDX11.h>

void ezViewContext::Redraw()
{
  ezGALDevice* pDevide = ezGALDevice::GetDefaultDevice();

  ezInt32 iOwnID = GetViewIndex();

  pDevide->BeginFrame();

  // The ezGALContext class is the main interaction point for draw / compute operations
  ezGALContext* pContext = pDevide->GetPrimaryContext();

  ezSizeU32 wndsize = GetEditorWindow().GetClientAreaSize();

  pContext->SetRenderTargetConfig(m_hBBRT);
  pContext->SetViewport(0.0f, 0.0f, (float) wndsize.width, (float) wndsize.height, 0.0f, 1.0f);

  static float fBlue = 0;
  fBlue = ezMath::Mod(fBlue + 0.01f, 1.0f);
  ezColor c(ezMath::Mod(0.3f * iOwnID, 1.0f), ezMath::Mod(0.1f * iOwnID, 1.0f), fBlue);
  pContext->Clear(c);

  pContext->SetRasterizerState(m_hRasterizerState);
  pContext->SetDepthStencilState(m_hDepthStencilState);

  pDevide->Present(m_hPrimarySwapChain);

  pDevide->EndFrame();
}

void ezEditorProcessApp::InitDevice()
{
  // Create a device
  ezGALDeviceCreationDescription DeviceInit;
  DeviceInit.m_bCreatePrimarySwapChain = false;
  DeviceInit.m_bDebugDevice = true;

  s_pDevice = EZ_DEFAULT_NEW(ezGALDeviceDX11)(DeviceInit);

  EZ_VERIFY(s_pDevice->Init() == EZ_SUCCESS, "Device init failed!");

  ezGALDevice::SetDefaultDevice(s_pDevice);
}