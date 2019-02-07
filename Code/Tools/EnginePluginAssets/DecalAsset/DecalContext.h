#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>

class EZ_ENGINEPLUGINASSETS_DLL ezDecalContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalContext, ezEngineProcessDocumentContext);

public:
  ezDecalContext();

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

private:
  //ezDecalResourceHandle m_hDecal;
};


