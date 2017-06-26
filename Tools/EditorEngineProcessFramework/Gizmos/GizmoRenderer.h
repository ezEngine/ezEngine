#pragma once

#include <EditorEngineProcessFramework/Plugin.h>
#include <RendererCore/Pipeline/Declarations.h>

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezGizmoRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmoRenderer, ezRenderer);

public:
  ezGizmoRenderer();
  ~ezGizmoRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

  static float s_fGizmoScale;

private:

  bool m_bEnabled;
  ezUInt32 m_uiHighlightID;
};

