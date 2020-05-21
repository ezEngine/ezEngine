#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/Renderer.h>

using ezShaderResourceHandle = ezTypedResourceHandle<class ezShaderResource>;

class EZ_RMLUIPLUGIN_DLL ezRmlUiRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRmlUiRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezRmlUiRenderer);

public:
  ezRmlUiRenderer();
  ~ezRmlUiRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const override;
  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& categories) const override;

  virtual void RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

private:
  ezShaderResourceHandle m_hShader;
  ezConstantBufferStorageHandle m_hConstantBuffer;

  ezVertexDeclarationInfo m_VertexDeclarationInfo;
};
