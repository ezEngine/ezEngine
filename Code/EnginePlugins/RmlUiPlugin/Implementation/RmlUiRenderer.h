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
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const override;

  virtual void RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

private:
  void SetScissorRect(const ezRenderViewContext& renderViewContext, const ezRectFloat& rect, bool bEnable, bool bTransformRect) const;
  void PrepareStencil(const ezRenderViewContext& renderViewContext, const ezRectFloat& rect) const;

  ezShaderResourceHandle m_hShader;
  ezConstantBufferStorageHandle m_hConstantBuffer;

  ezGALBufferHandle m_hQuadIndexBuffer;

  ezVertexDeclarationInfo m_VertexDeclarationInfo;

  mutable ezMat4 m_mLastTransform = ezMat4::MakeIdentity();
  mutable ezRectFloat m_LastRect = ezRectFloat(0, 0);
};
