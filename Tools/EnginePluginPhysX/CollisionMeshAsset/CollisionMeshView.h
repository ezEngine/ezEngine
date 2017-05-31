#pragma once

#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>

class ezCollisionMeshContext;

class ezCollisionMeshViewContext : public ezEngineProcessViewContext
{
public:
  ezCollisionMeshViewContext(ezCollisionMeshContext* pMeshContext);
  ~ezCollisionMeshViewContext();

  bool UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds);

protected:
  virtual ezViewHandle CreateView() override;

  ezCollisionMeshContext* m_pMeshContext;
};

