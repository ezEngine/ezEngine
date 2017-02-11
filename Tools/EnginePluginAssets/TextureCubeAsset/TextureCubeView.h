#pragma once

#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>

class ezTextureCubeContext;

class ezTextureCubeViewContext : public ezEngineProcessViewContext
{
public:
  ezTextureCubeViewContext(ezTextureCubeContext* pMaterialContext);
  ~ezTextureCubeViewContext();

protected:
  virtual ezView* CreateView() override;
  virtual void SetCamera(const ezViewRedrawMsgToEngine* pMsg) override;

  ezTextureCubeContext* m_pTextureContext;
};

