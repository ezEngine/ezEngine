#pragma once

#include <EnginePluginAssets/EnginePluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>

struct ezRenderPipelineContextLoaderConnection
{
  ezUuid m_Source;
  ezUuid m_Target;
  ezString m_SourcePin;
  ezString m_TargetPin;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezRenderPipelineContextLoaderConnection);


class ezRenderPipelineRttiConverterContext : public ezWorldRttiConverterContext
{
public:
  const ezRTTI* FindTypeByName(ezStringView sName) const override;
};

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

  virtual ezWorldRttiConverterContext& GetContext() override;
  virtual const ezWorldRttiConverterContext& GetContext() const override;

private:
  ezRenderPipelineRttiConverterContext m_RenderPipelineContext;
};
