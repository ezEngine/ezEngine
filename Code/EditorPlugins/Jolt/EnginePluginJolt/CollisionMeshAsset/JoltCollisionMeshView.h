#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class ezJoltCollisionMeshContext;

class ezJoltCollisionMeshViewContext : public ezEngineProcessViewContext
{
public:
  ezJoltCollisionMeshViewContext(ezJoltCollisionMeshContext* pMeshContext);
  ~ezJoltCollisionMeshViewContext();

  bool UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds);

protected:
  virtual ezViewHandle CreateView() override;
  virtual void SetCamera(const ezViewRedrawMsgToEngine* pMsg) override;

  ezJoltCollisionMeshContext* m_pContext = nullptr;
};
