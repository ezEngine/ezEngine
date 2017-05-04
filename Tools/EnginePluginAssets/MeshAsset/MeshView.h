#pragma once

#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>

class ezMeshContext;

class ezMeshViewContext : public ezEngineProcessViewContext
{
public:
  ezMeshViewContext(ezMeshContext* pMeshContext);
  ~ezMeshViewContext();

  bool UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds);

protected:
  virtual ezViewHandle CreateView() override;

  ezMeshContext* m_pMeshContext;
};

