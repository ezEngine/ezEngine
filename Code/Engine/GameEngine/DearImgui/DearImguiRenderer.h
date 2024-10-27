#pragma once

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Core/ResourceManager/ResourceHandle.h>
#  include <Foundation/Math/Rect.h>
#  include <GameEngine/GameEngineDLL.h>
#  include <Imgui/imgui.h>
#  include <RendererCore/Meshes/MeshBufferResource.h>
#  include <RendererCore/Pipeline/Extractor.h>
#  include <RendererCore/Pipeline/RenderData.h>
#  include <RendererCore/Pipeline/Renderer.h>
#  include <RendererFoundation/Resources/BufferPool.h>

class ezRenderDataBatch;
using ezShaderResourceHandle = ezTypedResourceHandle<class ezShaderResource>;

struct alignas(16) ezImguiVertex
{
  EZ_DECLARE_POD_TYPE();

  ezVec3 m_Position;
  ezVec2 m_TexCoord;
  ezColorLinearUB m_Color;
};

struct ezImguiBatch
{
  EZ_DECLARE_POD_TYPE();

  ezRectU32 m_ScissorRect;
  ezUInt16 m_uiTextureID;
  ezUInt16 m_uiVertexCount;
};

class EZ_GAMEENGINE_DLL ezImguiRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezImguiRenderData, ezRenderData);

public:
  ezArrayPtr<ezImguiVertex> m_Vertices;
  ezArrayPtr<ImDrawIdx> m_Indices;
  ezArrayPtr<ezImguiBatch> m_Batches;
};

class EZ_GAMEENGINE_DLL ezImguiExtractor : public ezExtractor
{
  EZ_ADD_DYNAMIC_REFLECTION(ezImguiExtractor, ezExtractor);

public:
  ezImguiExtractor(const char* szName = "ImguiExtractor");

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) override;
  virtual void PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData) override {}

  virtual ezResult Serialize(ezStreamWriter& inout_stream) const override;
  virtual ezResult Deserialize(ezStreamReader& inout_stream) override;
};

class EZ_GAMEENGINE_DLL ezImguiRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezImguiRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezImguiRenderer);

public:
  ezImguiRenderer();
  ~ezImguiRenderer();

  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;

protected:
  void SetupRenderer();

  static constexpr ezUInt32 s_uiVertexBufferSize = 10000;
  static constexpr ezUInt32 s_uiIndexBufferSize = s_uiVertexBufferSize * 2;

  ezShaderResourceHandle m_hShader;
  ezGALBufferPool m_VertexBuffer;
  ezGALBufferPool m_IndexBuffer;
  ezVertexDeclarationInfo m_VertexDeclarationInfo;
};

#endif
