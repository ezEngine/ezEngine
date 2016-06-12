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

  virtual void HandleViewMessage(const ezEditorEngineViewMsg* pMsg) override;

protected:
  virtual void Redraw() override;
  virtual void SetCamera(const ezViewRedrawMsgToEngine* pMsg) override;
  virtual ezView* CreateView() override;
  virtual bool IsDefaultRenderPipeline(ezRenderPipelineResourceHandle hPipeline) override;
  ezRenderPipelineResourceHandle CreateDefaultRenderPipeline() override;

  void PickObjectAt(ezUInt16 x, ezUInt16 y);

private:
  ezSceneContext* m_pSceneContext;

  bool m_bUpdatePickingData;


  ezPickingRenderPass* m_pPickingRenderPass;
};

