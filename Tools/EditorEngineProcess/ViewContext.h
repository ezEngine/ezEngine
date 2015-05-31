#pragma once

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>
#include <System/Window/Window.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <CoreUtils/Graphics/Camera.h>
#include <CoreUtils/Debugging/DataTransfer.h>

class ezView;
class ezViewCameraMsgToEngine;

struct ObjectData
{
  ezMat4 m_ModelView;
  float m_PickingID[4];
};

class ezViewContext : public ezEngineProcessViewContext
{
public:
  ezViewContext(ezInt32 iViewIndex, ezUuid DocumentGuid) : ezEngineProcessViewContext(iViewIndex, DocumentGuid)
  {
    m_pView = nullptr;
  }

  void SetupRenderTarget(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight);

  void Redraw();

  void SetCamera(ezViewCameraMsgToEngine* pMsg);

  void PickObjectAt(ezUInt16 x, ezUInt16 y);

  void SendViewMessage(ezEditorEngineDocumentMsg* pViewMsg);

private:
  ezGALRenderTargetConfigHandle m_hBBRT;
  ezGALSwapChainHandle m_hPrimarySwapChain;

  ezGALTextureHandle m_hPickingRT;
  ezGALTextureHandle m_hPickingDepthRT;
  ezGALRenderTargetConfigHandle m_hPickingRenderTargetCfg;

  ezCamera m_Camera;
  ezView* m_pView;

  ezMat4 m_PickingInverseViewProjectionMatrix;
  ezDynamicArray<float> m_PickingResultsDepth;
  ezDynamicArray<ezUInt32> m_PickingResultsComponentID;
  ezDynamicArray<ezUInt16> m_PickingResultsPartIndex;
};

