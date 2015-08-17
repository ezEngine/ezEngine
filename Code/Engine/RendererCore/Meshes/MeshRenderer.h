#pragma once

#include <RendererCore/Pipeline/Declarations.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezResourceHandle<class ezConstantBufferResource> ezConstantBufferResourceHandle;

class EZ_RENDERERCORE_DLL ezMeshRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshRenderer);

public:
  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual ezUInt32 Render(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezArrayPtr<const ezRenderData* const>& renderData) override;

private:
  ezConstantBufferResourceHandle m_hObjectTransformCB;
};

