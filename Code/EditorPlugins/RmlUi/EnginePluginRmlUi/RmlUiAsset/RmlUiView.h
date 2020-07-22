#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class ezRmlUiContext;

class ezRmlUiViewContext : public ezEngineProcessViewContext
{
public:
  ezRmlUiViewContext(ezRmlUiContext* pRmlUiContext);
  ~ezRmlUiViewContext();

  bool UpdateThumbnailCamera(const ezBoundingBoxSphere& bounds);

protected:
  virtual ezViewHandle CreateView() override;
  virtual void SetCamera(const ezViewRedrawMsgToEngine* pMsg) override;

  ezRmlUiContext* m_pRmlUiContext;
};
