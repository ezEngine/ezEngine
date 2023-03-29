#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class ezAnimatedMeshContext;

class ezAnimatedMeshViewContext : public ezEngineProcessViewContext
{
public:
  ezAnimatedMeshViewContext(ezAnimatedMeshContext* pMeshContext);
  ~ezAnimatedMeshViewContext();

  bool UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds);

protected:
  virtual ezViewHandle CreateView() override;
  virtual void SetCamera(const ezViewRedrawMsgToEngine* pMsg) override;

  ezAnimatedMeshContext* m_pContext = nullptr;
};
