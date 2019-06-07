#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class ezMeshContext;

class ezMeshViewContext : public ezEngineProcessViewContext
{
public:
  ezMeshViewContext(ezMeshContext* pMeshContext);
  ~ezMeshViewContext();

  bool UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds);

protected:
  virtual ezViewHandle CreateView() override;
  virtual void SetCamera(const ezViewRedrawMsgToEngine* pMsg) override;

  ezMeshContext* m_pMeshContext;


};

