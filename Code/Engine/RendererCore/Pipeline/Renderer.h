#pragma once

#include <RendererCore/Pipeline/RenderData.h>

/// \brief This is the base class for types that handle rendering of different object types.
///
/// E.g. there are different renderers for meshes, particle effects, light sources, etc.
class EZ_RENDERERCORE_DLL ezRenderer : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRenderer, ezReflectedClass);

public:
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const = 0;
  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& categories) const = 0;

  virtual void RenderBatch(
    const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const = 0;
};
