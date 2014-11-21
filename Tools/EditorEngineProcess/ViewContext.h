#pragma once

#include <EditorFramework/EditorApp.moc.h>
#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>
#include <System/Window/Window.h>
#include <RendererFoundation/Device/Device.h>

class ezViewContext : public ezEngineProcessViewContext
{
public:
  ezViewContext(ezInt32 iViewIndex, ezUuid DocumentGuid) : ezEngineProcessViewContext(iViewIndex, DocumentGuid) { }

  void SetupRenderTarget(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight);

  void Redraw();

private:
  ezGALRenderTargetConfigHandle m_hBBRT;
  ezGALBufferHandle m_hCB;
  ezGALRasterizerStateHandle m_hRasterizerState;
  ezGALDepthStencilStateHandle m_hDepthStencilState;
  ezGALSwapChainHandle m_hPrimarySwapChain;
};