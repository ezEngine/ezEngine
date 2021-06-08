#include <GameEnginePCH.h>

#ifdef BUILDSYSTEM_ENABLE_IMGUI_SUPPORT

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
}

void ezImguiExtractor::Extract(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& extractedRenderData)
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
      pRenderData->m_GlobalBounds.SetInvalid();

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
          batch.m_uiVertexCount = pCmd->ElemCount;
          batch.m_uiTextureID = (ezUInt16)iTextureID;
          batch.m_ScissorRect = ezRectU32((ezUInt32)pCmd->ClipRect.x, (ezUInt32)pCmd->ClipRect.y, (ezUInt32)(pCmd->ClipRect.z - pCmd->ClipRect.x), (ezUInt32)(pCmd->ClipRect.w - pCmd->ClipRect.y));
        }
      }

      extractedRenderData.AddRenderData(pRenderData, ezDefaultRenderDataCategories::GUI);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

ezImguiRenderer::ezImguiRenderer()
{
  SetupRenderer();
}

ezImguiRenderer::~ezImguiRenderer()
{
  m_hShader.Invalidate();

  if (!m_hVertexBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hVertexBuffer);
    m_hVertexBuffer.Invalidate();
  }

  if (!m_hIndexBuffer.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyBuffer(m_hIndexBuffer);
    m_hIndexBuffer.Invalidate();
  }
}

void ezImguiRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const
{
  types.PushBack(ezGetStaticRTTI<ezImguiRenderData>());
}

void ezImguiRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& categories) const
{
  categories.PushBack(ezDefaultRenderDataCategories::GUI);
}

void ezImguiRenderer::RenderBatch(const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  if (ezImgui::GetSingleton() == nullptr)
    return;

  ezRenderContext* pRenderContext = renderContext.m_pRenderContext;
  ezGALRenderCommandEncoder* pCommandEncoder = pRenderContext->GetRenderCommandEncoder();

  pRenderContext->BindShader(m_hShader);
  const auto& textures = ezImgui::GetSingleton()->m_hTextures;
  const ezUInt32 numTextures = textures.GetCount();

  for (auto it = batch.GetIterator<ezImguiRenderData>(); it.IsValid(); ++it)
  {
    const ezImguiRenderData* pRenderData = it;

    EZ_ASSERT_DEV(pRenderData->m_Vertices.GetCount() < VertexBufferSize, "GUI has too many elements to render in one drawcall");
    EZ_ASSERT_DEV(pRenderData->m_Indices.GetCount() < IndexBufferSize, "GUI has too many elements to render in one drawcall");

    pCommandEncoder->UpdateBuffer(m_hVertexBuffer, 0, ezMakeArrayPtr(pRenderData->m_Vertices.GetPtr(), pRenderData->m_Vertices.GetCount()).ToByteArray());
    pCommandEncoder->UpdateBuffer(m_hIndexBuffer, 0, ezMakeArrayPtr(pRenderData->m_Indices.GetPtr(), pRenderData->m_Indices.GetCount()).ToByteArray());

    pRenderContext->BindMeshBuffer(m_hVertexBuffer, m_hIndexBuffer, &m_VertexDeclarationInfo, ezGALPrimitiveTopology::Triangles, pRenderData->m_Indices.GetCount() / 3);

    ezUInt32 uiFirstIndex = 0;
    const ezUInt32 numBatches = pRenderData->m_Batches.GetCount();
    for (ezUInt32 batchIdx = 0; batchIdx < numBatches; ++batchIdx)
    {
      const ezImguiBatch& imGuiBatch = pRenderData->m_Batches[batchIdx];

      if (imGuiBatch.m_uiVertexCount > 0 && imGuiBatch.m_uiTextureID < numTextures)
      {
        pCommandEncoder->SetScissorRect(imGuiBatch.m_ScissorRect);
        pRenderContext->BindTexture2D("BaseTexture", textures[imGuiBatch.m_uiTextureID]);
        pRenderContext->DrawMeshBuffer(imGuiBatch.m_uiVertexCount / 3, uiFirstIndex / 3).IgnoreResult();
      }

      uiFirstIndex += imGuiBatch.m_uiVertexCount;
    }
  }
}

void ezImguiRenderer::SetupRenderer()
{
  if (!m_hVertexBuffer.IsInvalidated())
    return;

  // load the shader
  {
    m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/GUI/DearImguiPrimitives.ezShader");
  }

  // Create the vertex buffer
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ezImguiVertex);
    desc.m_uiTotalSize = VertexBufferSize * desc.m_uiStructSize;
    desc.m_BufferType = ezGALBufferType::VertexBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hVertexBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  // Create the index buffer
  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(ImDrawIdx);
    desc.m_uiTotalSize = IndexBufferSize * desc.m_uiStructSize;
    desc.m_BufferType = ezGALBufferType::IndexBuffer;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hIndexBuffer = ezGALDevice::GetDefaultDevice()->CreateBuffer(desc);
  }

  // Setup the vertex declaration
  {
    {
      ezVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = ezGALVertexAttributeSemantic::Position;
      si.m_Format = ezGALResourceFormat::XYZFloat;
      si.m_uiOffset = 0;
      si.m_uiElementSize = 12;
    }

    {
      ezVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = ezGALVertexAttributeSemantic::TexCoord0;
      si.m_Format = ezGALResourceFormat::UVFloat;
      si.m_uiOffset = 12;
      si.m_uiElementSize = 8;
    }

    {
      ezVertexStreamInfo& si = m_VertexDeclarationInfo.m_VertexStreams.ExpandAndGetRef();
      si.m_Semantic = ezGALVertexAttributeSemantic::Color0;
      si.m_Format = ezGALResourceFormat::RGBAUByteNormalized;
      si.m_uiOffset = 20;
      si.m_uiElementSize = 4;
    }
  }
}

#endif

EZ_STATICLINK_FILE(GameEngine, GameEngine_DearImgui_Implementation_DearImguiRenderer);
