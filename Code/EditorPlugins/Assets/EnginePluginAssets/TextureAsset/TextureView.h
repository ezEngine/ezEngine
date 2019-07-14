#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class ezTextureContext;

class ezTextureViewContext : public ezEngineProcessViewContext
{
public:
  ezTextureViewContext(ezTextureContext* pMaterialContext);
  ~ezTextureViewContext();

protected:
  virtual ezViewHandle CreateView() override;
  virtual void SetCamera(const ezViewRedrawMsgToEngine* pMsg) override;

  ezTextureContext* m_pTextureContext;
};

