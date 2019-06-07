#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class ezKrautTreeContext;

class ezKrautTreeViewContext : public ezEngineProcessViewContext
{
public:
  ezKrautTreeViewContext(ezKrautTreeContext* pKrautTreeContext);
  ~ezKrautTreeViewContext();

  bool UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds);

protected:
  virtual ezViewHandle CreateView() override;
  virtual void SetCamera(const ezViewRedrawMsgToEngine* pMsg) override;

  ezKrautTreeContext* m_pKrautTreeContext;
};

