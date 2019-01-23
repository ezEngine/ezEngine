#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class ezDecalContext;

class ezDecalViewContext : public ezEngineProcessViewContext
{
public:
  ezDecalViewContext(ezDecalContext* pDecalContext);
  ~ezDecalViewContext();

protected:
  virtual ezViewHandle CreateView() override;

  ezDecalContext* m_pDecalContext;
};

