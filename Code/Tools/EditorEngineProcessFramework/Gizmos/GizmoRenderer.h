#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <RendererCore/Pipeline/Renderer.h>

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezGizmoRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGizmoRenderer, ezRenderer);

public:
  ezGizmoRenderer();
  ~ezGizmoRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const override;
  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& categories) const override;
  virtual void RenderBatch(
    const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

  static float s_fGizmoScale;
};
