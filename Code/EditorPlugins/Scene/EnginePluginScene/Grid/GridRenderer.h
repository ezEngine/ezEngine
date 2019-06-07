#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/Renderer.h>

class ezRenderDataBatch;
class ezSceneContext;

typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;

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

  virtual void Extract(
    const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& extractedRenderData) override;

  void SetSceneContext(ezSceneContext* pSceneContext) { m_pSceneContext = pSceneContext; }
  ezSceneContext* GetSceneContext() const { return m_pSceneContext; }

private:
  ezSceneContext* m_pSceneContext;
};

struct EZ_ALIGN_16(GridVertex)
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
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const override;
  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& categories) const override;
  virtual void RenderBatch(
    const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

protected:
  void CreateVertexBuffer();

  static const ezUInt32 BufferSize = 1024 * 8;
  static const ezUInt32 LineVerticesPerBatch = BufferSize / sizeof(GridVertex);

  ezShaderResourceHandle m_hShader;
  ezGALBufferHandle m_hVertexBuffer;
  ezVertexDeclarationInfo m_VertexDeclarationInfo;
  mutable ezDynamicArray<GridVertex, ezAlignedAllocatorWrapper> m_Vertices;

private:
  void CreateGrid(const ezGridRenderData& rd) const;
};
