#pragma once

#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>

class ezTextureContext;

class ezTextureViewContext : public ezEngineProcessViewContext
{
public:
  ezTextureViewContext(ezTextureContext* pMaterialContext);
  ~ezTextureViewContext();

protected:
  virtual ezView* CreateView() override;
  virtual void SetCamera(const ezViewRedrawMsgToEngine* pMsg) override;

  ezTextureContext* m_pTextureContext;
};

