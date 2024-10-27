#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/Renderer.h>
#include <RendererFoundation/Resources/BufferPool.h>

class ezRenderDataBatch;
class ezSceneContext;

using ezShaderResourceHandle = ezTypedResourceHandle<class ezShaderResource>;

class ezGridRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGridRenderData, ezRenderData);

public:
  float m_fDensity;
  ezInt32 m_iFirstLine1;
  ezInt32 m_iLastLine1;
  ezInt32 m_iFirstLine2;
  ezInt32 m_iLastLine2;
  bool m_bOrthoMode;
  bool m_bGlobal;
};

class ezEditorGridExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEditorGridExtractor, ezExtractor);

public:
  ezEditorGridExtractor(const char* szName = "EditorGridExtractor");

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) override;
  virtual void PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) override {}

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;

  void SetSceneContext(ezSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  ezSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  ezSceneContext* m_pSceneContext;
};

struct alignas(16) GridVertex
{
  ezVec3 m_position;
  ezColorLinearUB m_color;
};

class ezGridRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezGridRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezGridRenderer);

public:
  ezGridRenderer();

  // ezRenderer implementation
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

protected:
  void CreateVertexBuffer();

  static constexpr ezUInt32 s_uiBufferSize = 1024 * 8;
  static constexpr ezUInt32 s_uiLineVerticesPerBatch = s_uiBufferSize / sizeof(GridVertex);

  ezShaderResourceHandle m_hShader;
  ezGALBufferPool m_VertexBuffer;
  ezVertexDeclarationInfo m_VertexDeclarationInfo;
  mutable ezDynamicArray<GridVertex, ezAlignedAllocatorWrapper> m_Vertices;

private:
  void CreateGrid(const ezGridRenderData& rd) const;
};
