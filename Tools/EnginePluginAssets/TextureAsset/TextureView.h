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

  ezTextureContext* m_pTextureContext;
};

