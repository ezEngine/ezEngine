#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class ezRmlUiDocumentContext;

class ezRmlUiViewContext : public ezEngineProcessViewContext
{
public:
  ezRmlUiViewContext(ezRmlUiDocumentContext* pRmlUiContext);
  ~ezRmlUiViewContext();

  bool UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds);

protected:
  virtual ezViewHandle CreateView() override;
  virtual void SetCamera(const ezViewRedrawMsgToEngine* pMsg) override;

  ezRmlUiDocumentContext* m_pRmlUiContext;
};
