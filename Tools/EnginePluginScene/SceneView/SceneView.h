#pragma once

#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>
#include <EnginePluginScene/PickingRenderPass/PickingRenderPass.h>
#include <CoreUtils/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezView;
class ezViewRedrawMsgToEngine;
class ezEngineProcessDocumentContext;
class ezEditorEngineDocumentMsg;
class ezEditorRenderPass;
class ezSelectedObjectsExtractor;
class ezSceneContext;
typedef ezTypedResourceHandle<class ezRenderPipelineResource> ezRenderPipelineResourceHandle;

struct ObjectData
{
  ezMat4 m_ModelView;
  float m_PickingID[4];
};

class ezSceneViewContext : public ezEngineProcessViewContext
{
public:
  ezSceneViewContext(ezSceneContext* pSceneContext);
  ~ezSceneViewContext();

  void SetupRenderTarget(ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight);

  void Redraw();

  void SetCamera(const ezViewRedrawMsgToEngine* pMsg);

  void PickObjectAt(ezUInt16 x, ezUInt16 y);

  void SendViewMessage(ezEditorEngineDocumentMsg* pViewMsg = false);

  virtual void HandleViewMessage(const ezEditorEngineViewMsg* pMsg) override;

private:
  void CreateView();

  ezRenderPipelineResourceHandle CreateEditorRenderPipeline();

  ezSceneContext* m_pSceneContext;

  ezCamera m_Camera;
  ezView* m_pView;
  bool m_bUpdatePickingData;


  ezPickingRenderPass* m_pPickingRenderPass;
};

