#pragma once

#include <GameEngine/Basics.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <Foundation/Math/Rect.h>
#include <ThirdParty/Imgui/imgui.h>

class ezRenderDataBatch;
typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;

struct EZ_ALIGN_16(ezImguiVertex)
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

  virtual void Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
    ezExtractedRenderData& extractedRenderData) override;
};

class EZ_GAMEENGINE_DLL ezImguiRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezImguiRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezImguiRenderer);

public:
  ezImguiRenderer();
  ~ezImguiRenderer();

  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) override;

protected:
  void SetupRenderer();

  static const ezUInt32 VertexBufferSize = 10000;
  static const ezUInt32 IndexBufferSize = VertexBufferSize * 2;

  ezShaderResourceHandle m_hShader;
  ezGALBufferHandle m_hVertexBuffer;
  ezGALBufferHandle m_hIndexBuffer;
  ezVertexDeclarationInfo m_VertexDeclarationInfo;
};

