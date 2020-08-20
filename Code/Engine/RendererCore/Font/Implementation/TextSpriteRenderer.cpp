#include <RendererCorePCH.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Font/TextSprite.h>
#include <RendererCore/Font/TextSpriteRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

struct EZ_ALIGN_16(ezTextSpriteData)
{
  ezVec3 m_worldSpacePosition;
  float m_size;
  float m_maxScreenSize;
  float m_aspectRatio;
  ezUInt32 m_colorRG;
  ezUInt32 m_colorBA;
  ezUInt32 m_texCoordScale;
  ezUInt32 m_texCoordOffset;
  ezUInt32 m_gameObjectID;
  ezUInt32 m_reserved;
};

EZ_CHECK_AT_COMPILETIME(sizeof(ezTextSpriteData) == 48);

namespace
{
  enum
  {
    MAX_SPRITE_DATA_PER_BATCH = 1024
  };
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextSpriteRenderer, 1, ezRTTIDefaultAllocator<ezTextSpriteRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTextSpriteRenderer::ezTextSpriteRenderer()
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Materials/TextSpriteMaterial.ezShader");
}

ezTextSpriteRenderer::~ezTextSpriteRenderer() {}

void ezTextSpriteRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const
{
  types.PushBack(ezGetStaticRTTI<ezTextSpriteRenderData>());
}

void ezTextSpriteRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& categories) const
{
  categories.PushBack(ezDefaultRenderDataCategories::LitMasked);
  categories.PushBack(ezDefaultRenderDataCategories::LitTransparent);
  categories.PushBack(ezDefaultRenderDataCategories::SimpleOpaque);
  categories.PushBack(ezDefaultRenderDataCategories::SimpleTransparent);
  categories.PushBack(ezDefaultRenderDataCategories::Selection);
}

void ezTextSpriteRenderer::RenderBatch(
  const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;
  ezGALContext* pGALContext = pContext->GetGALContext();

  const ezTextSpriteRenderData* pRenderData = batch.GetFirstData<ezTextSpriteRenderData>();

  ezGALBufferHandle hTextSpriteData = CreateTextSpriteDataBuffer();
  EZ_SCOPE_EXIT(DeleteTextSpriteDataBuffer(hTextSpriteData));

  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindBuffer("textSpriteData", pDevice->GetDefaultResourceView(hTextSpriteData));
  renderViewContext.m_pRenderContext->BindTexture2D("TextSpriteTexture", pRenderData->m_hTexture);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLEND_MODE", ezTextSpriteBlendMode::GetPermutationValue(pRenderData->m_BlendMode));

  ezUInt32 uiStartIndex = 0;
  while (uiStartIndex < batch.GetCount())
  {
    const ezUInt32 uiCount = ezMath::Min(batch.GetCount() - uiStartIndex, (ezUInt32)MAX_SPRITE_DATA_PER_BATCH);

    FillTextSpriteData(batch, uiStartIndex, uiCount);
    if (m_TextSpriteData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
    {
      pGALContext->UpdateBuffer(hTextSpriteData, 0, m_TextSpriteData.GetByteArrayPtr());

      renderViewContext.m_pRenderContext->BindMeshBuffer(
        ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, uiCount * 2);
      renderViewContext.m_pRenderContext->DrawMeshBuffer();
    }

    uiStartIndex += uiCount;
  }
}

ezGALBufferHandle ezTextSpriteRenderer::CreateTextSpriteDataBuffer() const
{
  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(ezTextSpriteData);
  desc.m_uiTotalSize = desc.m_uiStructSize * MAX_SPRITE_DATA_PER_BATCH;
  desc.m_BufferType = ezGALBufferType::Generic;
  desc.m_bUseAsStructuredBuffer = true;
  desc.m_bAllowShaderResourceView = true;
  desc.m_ResourceAccess.m_bImmutable = false;

  return ezGPUResourcePool::GetDefaultInstance()->GetBuffer(desc);
}

void ezTextSpriteRenderer::DeleteTextSpriteDataBuffer(ezGALBufferHandle hBuffer) const
{
  ezGPUResourcePool::GetDefaultInstance()->ReturnBuffer(hBuffer);
}

void ezTextSpriteRenderer::FillTextSpriteData(const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32 uiCount) const
{
  m_TextSpriteData.Clear();
  m_TextSpriteData.Reserve(uiCount);

  for (auto it = batch.GetIterator<ezTextSpriteRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    const ezTextSpriteRenderData* pRenderData = it;

    auto& textSpriteData = m_TextSpriteData.ExpandAndGetRef();

    textSpriteData.m_worldSpacePosition = pRenderData->m_GlobalTransform.m_vPosition;
    textSpriteData.m_size = pRenderData->m_fSize;
    textSpriteData.m_maxScreenSize = pRenderData->m_fMaxScreenSize;
    textSpriteData.m_aspectRatio = pRenderData->m_fAspectRatio;
    textSpriteData.m_colorRG = ezShaderUtils::Float2ToRG16F(ezVec2(pRenderData->m_color.r, pRenderData->m_color.g));
    textSpriteData.m_colorBA = ezShaderUtils::Float2ToRG16F(ezVec2(pRenderData->m_color.b, pRenderData->m_color.a));
    textSpriteData.m_texCoordScale = ezShaderUtils::Float2ToRG16F(pRenderData->m_texCoordScale);
    textSpriteData.m_texCoordOffset = ezShaderUtils::Float2ToRG16F(pRenderData->m_texCoordOffset);
    textSpriteData.m_gameObjectID = pRenderData->m_uiUniqueID;
    textSpriteData.m_reserved = 0;
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Font_Implementation_TextSpriteRenderer);
