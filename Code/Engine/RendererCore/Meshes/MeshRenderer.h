#pragma once

#include <RendererCore/Pipeline/Declarations.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezResourceHandle<class ezConstantBufferResource> ezConstantBufferResourceHandle;

class EZ_RENDERERCORE_DLL ezMeshRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshRenderer, ezRenderer);

public:
  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezArrayPtr<const ezRenderData* const>& batch) override;

private:
  ezConstantBufferResourceHandle m_hObjectTransformCB;
};

