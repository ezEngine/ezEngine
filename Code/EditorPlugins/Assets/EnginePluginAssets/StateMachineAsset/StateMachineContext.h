#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>

class EZ_ENGINEPLUGINASSETS_DLL ezStateMachineContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezStateMachineContext, ezEngineProcessDocumentContext);

public:
  ezStateMachineContext();

protected:
  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

  virtual ezStatus ExportDocument(const ezExportDocumentMsgToEngine* pMsg) override;
};
