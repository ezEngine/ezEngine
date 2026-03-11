#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class ezMeshContext;

class ezMeshViewContext : public ezEngineProcessViewContext
{
public:
  ezMeshViewContext(ezMeshContext* pMeshContext);
  ~ezMeshViewContext();

  bool UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds);

  virtual void HandleViewMessage(const ezEditorEngineViewMsg* pMsg) override;

protected:
  virtual ezViewHandle CreateView() override;
  virtual void SetCamera(const ezViewRedrawMsgToEngine* pMsg) override;

  void PickObjectAt(ezUInt16 x, ezUInt16 y);

  ezMeshContext* m_pContext = nullptr;
  ezUInt32 m_uiLastHoveredPartIndex = ezInvalidIndex;
};
