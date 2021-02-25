#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class ezSkeletonContext;

class ezSkeletonViewContext : public ezEngineProcessViewContext
{
public:
  ezSkeletonViewContext(ezSkeletonContext* pContext);
  ~ezSkeletonViewContext();

  bool UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds);

protected:
  virtual ezViewHandle CreateView() override;
  virtual void SetCamera(const ezViewRedrawMsgToEngine* pMsg) override;

  ezSkeletonContext* m_pContext;
};
