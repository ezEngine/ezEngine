#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>

class EZ_ENGINEPLUGINASSETS_DLL ezRenderPipelineContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderPipelineContext, ezEngineProcessDocumentContext);

public:
  ezRenderPipelineContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

  virtual ezStatus ExportDocument(const ezExportDocumentMsgToEngine* pMsg) override;
};
