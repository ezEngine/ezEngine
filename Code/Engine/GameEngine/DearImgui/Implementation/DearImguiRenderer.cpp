#include <GameEngine/GameEnginePCH.h>

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

#  include <Foundation/IO/TypeVersionContext.h>
#  include <GameEngine/DearImgui/DearImgui.h>
#  include <GameEngine/DearImgui/DearImguiRenderer.h>
#  include <Imgui/imgui_internal.h>
#  include <RendererCore/Pipeline/ExtractedRenderData.h>
#  include <RendererCore/Pipeline/View.h>
#  include <RendererCore/RenderContext/RenderContext.h>
#  include <RendererCore/RenderWorld/RenderWorld.h>
#  include <RendererCore/Shader/ShaderResource.h>
#  include <RendererFoundation/Device/Device.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezImguiRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezImguiExtractor, 1, ezRTTIDefaultAllocator<ezImguiExtractor>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezImguiRenderer, 1, ezRTTIDefaultAllocator<ezImguiRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezImguiExtractor::ezImguiExtractor(const char* szName)
  : ezExtractor(szName)
{
  m_DependsOn.PushBack(ezMakeHashedString("ezVisibleObjectsExtractor"));
}

void ezImguiExtractor::Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData)
{
  ezImgui* pImGui = ezImgui::GetSingleton();
  if (pImGui == nullptr)
  {
    return;
  }

  {
    EZ_LOCK(pImGui->m_ViewToContextTableMutex);
    ezImgui::Context context;
    if (!pImGui->m_ViewToContextTable.TryGetValue(view.GetHandle(), context))
    {
      // No context for this view
      return;
    }

    ezUInt64 uiCurrentFrameCounter = ezRenderWorld::GetFrameCounter();
    if (context.m_uiFrameBeginCounter != uiCurrentFrameCounter)
    {
      // Nothing was rendered with ImGui this frame
      return;
    }

    context.m_uiFrameRenderCounter = uiCurrentFrameCounter;

    ImGui::SetCurrentContext(context.m_pImGuiContext);
  }

  ImGui::Render();

  ImDrawData* pDrawData = ImGui::GetDrawData();

  if (pDrawData && pDrawData->Valid)
  {
    for (int draw = 0; draw < pDrawData->CmdListsCount; ++draw)
    {
      ezImguiRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezImguiRenderData>(nullptr);
      pRenderData->m_uiSortingKey = draw;
      pRenderData->m_GlobalTransform.SetIdentity();
      pRenderData->m_GlobalBounds = ezBoundingBoxSphere::MakeInvalid();

      // copy the vertex data
      // uses the frame allocator to prevent unnecessary deallocations
      {
        const ImDrawList* pCmdList = pDrawData->CmdLists[draw];

        pRenderData->m_Vertices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezImguiVertex, pCmdList->VtxBuffer.size());
        for (ezUInt32 vtx = 0; vtx < pRenderData->m_Vertices.GetCount(); ++vtx)
        {
          const auto& vert = pCmdList->VtxBuffer[vtx];

          pRenderData->m_Vertices[vtx].m_Position.Set(vert.pos.x, vert.pos.y, 0);
          pRenderData->m_Vertices[vtx].m_TexCoord.Set(vert.uv.x, vert.uv.y);
          pRenderData->m_Vertices[vtx].m_Color = *reinterpret_cast<const ezColorGammaUB*>(&vert.col);
        }

        pRenderData->m_Indices = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ImDrawIdx, pCmdList->IdxBuffer.size());
        for (ezUInt32 i = 0; i < pRenderData->m_Indices.GetCount(); ++i)
        {
          pRenderData->m_Indices[i] = pCmdList->IdxBuffer[i];
        }
      }

      // pass along an ezImguiBatch for every necessary drawcall
      {
        const ImDrawList* pCommands = pDrawData->CmdLists[draw];

        pRenderData->m_Batches = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezImguiBatch, pCommands->CmdBuffer.Size);

        for (int cmdIdx = 0; cmdIdx < pCommands->CmdBuffer.Size; cmdIdx++)
        {
          const ImDrawCmd* pCmd = &pCommands->CmdBuffer[cmdIdx];
          const size_t iTextureID = reinterpret_cast<size_t>(pCmd->TextureId);

          ezImguiBatch& batch = pRenderData->m_Batches[cmdIdx];
          batch.m_uiVertexCount = static_cast<ezUInt16>(pCmd->ElemCount);
          batch.m_uiTextureID = (ezUInt16)iTextureID;
          batch.m_ScissorRect = ezRectU32((ezUInt32)pCmd->ClipRect.x, (ezUInt32)pCmd->ClipRect.y, (ezUInt32)(pCmd->ClipRect.z - pCmd->ClipRect.x), (ezUInt32)(pCmd->ClipRect.w - pCmd->ClipRect.y));
        }
      }

      ref_extractedRenderData.AddRenderData(pRenderData, ezDefaultRenderDataCategories::GUI);
    }
  }
}

ezResult ezImguiExtractor::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return EZ_SUCCESS;
}

ezResult ezImguiExtractor::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

ezImguiRenderer::ezImguiRenderer()
{
  SetupRenderer();
}

ezImguiRenderer::~ezImguiRenderer()
{
  m_hShader.Invalidate();
}

void ezImguiRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezImguiRenderData>());
}

void ezImguiRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(ezDefaultRenderDataCategories::GUI);
}

void ezImguiRenderer::RenderBatch(const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  if (ezImgui::GetSingleton() == nullptr)
    return;

  ezRenderContext* pRenderContext = renderContext.m_pRenderContext;
  ezGALCommandEncoder* pCommandEncoder = pRenderContext->GetCommandEncoder();

  pRenderContext->BindShader(m_hShader);
  const auto& textures = ezImgui::GetSingleton()->m_Textures;
  const ezUInt32 numTextures = textures.GetCount();
  ezBindGroupBuilder& bindGroup = pRenderContext->GetBindGroup();
  for (auto it = batch.GetIterator<ezImguiRenderData>(); it.IsValid(); ++it)
  {
    const ezImguiRenderData* pRenderData = it;

    EZ_ASSERT_DEV(pRenderData->m_Vertices.GetCount() < s_uiVertexBufferSize, "GUI has too many elements to render in one drawcall");
    EZ_ASSERT_DEV(pRenderData->m_Indices.GetCount() < s_uiIndexBufferSize, "GUI has too many elements to render in one drawcall");

    ezGALBufferHandle hVertexBuffer = m_VertexBuffer.GetNewBuffer();
    ezGALBufferHandle hIndexBuffer = m_IndexBuffer.GetNewBuffer();

    pCommandEncoder->UpdateBuffer(hVertexBuffer, 0, ezMakeArrayPtr(pRenderData->m_Vertices.GetPtr(), pRenderData->m_Vertices.GetCount()).ToByteArray(), ezGALUpdateMode::AheadOfTime);
    pCommandEncoder->UpdateBuffer(hIndexBuffer, 0, ezMakeArrayPtr(pRenderData->m_Indices.GetPtr(), pRenderData->m_Indices.GetCount()).ToByteArray(), ezGALUpdateMode::AheadOfTime);

    pRenderContext->BindMeshBuffer(ezMakeArrayPtr(&hVertexBuffer, 1), hIndexBuffer, m_VertexAttributes, ezGALPrimitiveTopology::Triangles, pRenderData->m_Indices.GetCount() / 3);

    ezUInt32 uiFirstIndex = 0;
    const ezUInt32 numBatches = pRenderData->m_Batches.GetCount();
    for (ezUInt32 batchIdx = 0; batchIdx < numBatches; ++batchIdx)
    {
      const ezImguiBatch& imGuiBatch = pRenderData->m_Batches[batchIdx];

      if (imGuiBatch.m_uiVertexCount > 0 && imGuiBatch.m_uiTextureID < numTextures)
      {
        pCommandEncoder->SetScissorRect(imGuiBatch.m_ScissorRect);
        bindGroup.BindTexture("BaseTexture", textures[imGuiBatch.m_uiTextureID]);
        pRenderContext->DrawMeshBuffer(imGuiBatch.m_uiVertexCount / 3, uiFirstIndex / 3).IgnoreResult();
      }

      uiFirstIndex += imGuiBatch.m_uiVertexCount;
    }
  }

  // Reset scissor to default.
  ezRectFloat rect = renderContext.m_pViewData->m_ViewPortRect;
  pCommandEncoder->SetScissorRect(ezRectU32((ezUInt32)rect.x, (ezUInt32)rect.y, (ezUInt32)rect.width, (ezUInt32)rect.height));
}

void ezImguiRenderer::SetupRenderer()
{
  if (m_hShader.IsValid())
    return;

  // load the shader
  {
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/GUI/DearImguiPrimitives.ezShader");
  }

  // Create the vertex buffer
  {
    ezGALBufferCreationDescription desc;
    desc.m_BufferFlags = ezGALBufferUsageFlags::VertexBuffer | ezGALBufferUsageFlags::Transient;

    desc.m_uiStructSize = sizeof(ezImguiVertex);
    desc.m_uiTotalSize = s_uiVertexBufferSize * desc.m_uiStructSize;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_VertexBuffer.Initialize(desc, "DearImguiRenderer - VertexBuffer");
  }

  // Create the index buffer
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ImDrawIdx);
    desc.m_uiTotalSize = s_uiIndexBufferSize * desc.m_uiStructSize;
    desc.m_BufferFlags = ezGALBufferUsageFlags::IndexBuffer | ezGALBufferUsageFlags::Transient;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_IndexBuffer.Initialize(desc, "DearImguiRenderer - IndexBuffer");
  }

  // Setup the vertex declaration
  {
    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = ezGALVertexAttributeSemantic::Position;
      va.m_eFormat = ezGALResourceFormat::XYZFloat;
      va.m_uiOffset = offsetof(ezImguiVertex, m_Position);
    }

    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = ezGALVertexAttributeSemantic::TexCoord0;
      va.m_eFormat = ezGALResourceFormat::UVFloat;
      va.m_uiOffset = offsetof(ezImguiVertex, m_TexCoord);
    }

    {
      auto& va = m_VertexAttributes.ExpandAndGetRef();
      va.m_eSemantic = ezGALVertexAttributeSemantic::Color0;
      va.m_eFormat = ezGALResourceFormat::RGBAUByteNormalized;
      va.m_uiOffset = offsetof(ezImguiVertex, m_Color);
    }
  }
}

#endif

EZ_STATICLINK_FILE(GameEngine, GameEngine_DearImgui_Implementation_DearImguiRenderer);
