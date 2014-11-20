#pragma once

#include <EditorFramework/EditorApp.moc.h>
#include <System/Window/Window.h>
#include <RendererFoundation/Device/Device.h>

class ezEditorProcessViewWindow : public ezWindowBase
{
public:
  ezEditorProcessViewWindow()
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

class ezViewContext
{
public:

  ezEditorProcessViewWindow& GetWindow() { return m_Window; }

  void SetupRenderTarget(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight);

  void Redraw(ezInt32 iOwnID);

private:
  ezEditorProcessViewWindow m_Window;

  ezGALRenderTargetConfigHandle m_hBBRT;
  ezGALBufferHandle m_hCB;
  ezGALRasterizerStateHandle m_hRasterizerState;
  ezGALDepthStencilStateHandle m_hDepthStencilState;
  ezGALSwapChainHandle m_hPrimarySwapChain;
};