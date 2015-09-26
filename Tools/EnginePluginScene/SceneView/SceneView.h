#pragma once

#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>
#include <EnginePluginScene/PickingRenderPass/PickingRenderPass.h>
#include <CoreUtils/Graphics/Camera.h>

class ezView;
class ezViewCameraMsgToEngine;
class ezEngineProcessDocumentContext;
class ezEditorEngineDocumentMsg;
class ezEditorRenderPass;

struct ObjectData
{
  ezMat4 m_ModelView;
  float m_PickingID[4];
};

class ezViewContext : public ezEngineProcessViewContext
{
public:
  ezViewContext(ezEngineProcessDocumentContext* pContext) : ezEngineProcessViewContext(pContext)
  {
    m_pView = nullptr;
    m_pEditorRenderPass = nullptr;
    m_pPickingRenderPass = nullptr;
    m_bUpdatePickingData = true;
  }

  void SetupRenderTarget(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight);

  void Redraw();

  void SetCamera(const ezViewCameraMsgToEngine* pMsg);

  void PickObjectAt(ezUInt16 x, ezUInt16 y);

  void SendViewMessage(ezEditorEngineDocumentMsg* pViewMsg, bool bSuperHighPriority = false);

  virtual void HandleViewMessage(const ezEditorEngineViewMsg* pMsg) override;

private:
  void RenderPassEventHandler(const ezPickingRenderPass::Event& e);

  ezGALSwapChainHandle m_hPrimarySwapChain;

  ezGALRenderTargetViewHandle m_hSwapChainRTV;
  ezGALRenderTargetViewHandle m_hSwapChainDSV;

  ezCamera m_Camera;
  ezView* m_pView;
  bool m_bUpdatePickingData;

  ezGALTextureHandle m_hPickingIdRT;
  ezGALTextureHandle m_hPickingDepthRT;

  ezGALRenderTargetViewHandle m_hPickingIdRTV;
  ezGALRenderTargetViewHandle m_hPickingDepthDSV;

  /// we need this matrix to compute the world space position of picked pixels
  ezMat4 m_PickingInverseViewProjectionMatrix;

  /// stores the 2D depth buffer image (32 Bit depth precision), to compute pixel positions from
  ezDynamicArray<float> m_PickingResultsDepth;

  /// Stores the 32 Bit picking ID values of each pixel. This can lead back to the ezComponent, etc. that rendered to that pixel
  ezDynamicArray<ezUInt32> m_PickingResultsID;

  /// Stores the optional/additional "part index" of a pickable object. E.g. for a mesh this can be the sub-mesh or material index
  ezDynamicArray<ezUInt16> m_PickingResultsPartIndex;

  ezPickingRenderPass* m_pPickingRenderPass;
  ezEditorRenderPass* m_pEditorRenderPass;
};

