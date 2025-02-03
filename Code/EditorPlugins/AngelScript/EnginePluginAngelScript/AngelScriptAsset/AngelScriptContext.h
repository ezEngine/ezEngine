#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginAngelScript/EnginePluginAngelScriptDLL.h>

class EZ_ENGINEPLUGINAS_DLL ezAngelScriptDocumentContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAngelScriptDocumentContext, ezEngineProcessDocumentContext);

public:
  ezAngelScriptDocumentContext();
  ~ezAngelScriptDocumentContext();

protected:
  virtual ezStatus ExportDocument(const ezExportDocumentMsgToEngine* pMsg) override;
  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;

  ezEngineProcessViewContext* CreateViewContext() override;
  void DestroyViewContext(ezEngineProcessViewContext* pContext) override;

  void SyncExposedParameters();
  asIScriptModule* CompileModule();
  void RetrieveScriptInfos(ezStringView sBasePath);

  ezString m_sInputFile;
  ezString m_sClass;
  ezString m_sCode;
};
