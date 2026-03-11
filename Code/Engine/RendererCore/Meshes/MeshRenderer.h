#pragma once

#include <RendererCore/Pipeline/Renderer.h>

class ezMeshRenderData;
struct ezPerInstanceData;

/// Implements rendering of static meshes.
///
/// All meshes in one batch are rendered with a single instanced draw call.
class EZ_RENDERERCORE_DLL ezMeshRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezMeshRenderer);

public:
  ezMeshRenderer();
  ~ezMeshRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezDynamicArray<const ezRTTI*>& out_types) const override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

protected:
  /// Sets additional shader data specific to the current render data.
  ///
  /// Can be overridden to bind custom per-object data.
  virtual void SetAdditionalData(const ezRenderViewContext& renderViewContext, const ezMeshRenderData* pRenderData) const;
};
