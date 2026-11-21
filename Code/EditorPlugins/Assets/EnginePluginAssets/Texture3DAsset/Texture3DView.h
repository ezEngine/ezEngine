#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class ezTexture3DContext;

class ezTexture3DViewContext : public ezEngineProcessViewContext
{
public:
  ezTexture3DViewContext(ezTexture3DContext* pTextureContext);
  ~ezTexture3DViewContext();

protected:
  virtual ezViewHandle CreateView() override;
  virtual void SetCamera(const ezViewRedrawMsgToEngine* pMsg) override;

  ezTexture3DContext* m_pTexture3DContext;
};
