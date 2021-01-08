#include <RendererCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Components/SpriteComponent.h>
#include <RendererCore/Components/SpriteRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

struct EZ_ALIGN_16(SpriteData)
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

EZ_CHECK_AT_COMPILETIME(sizeof(SpriteData) == 48);

namespace
{
  enum
  {
    MAX_SPRITE_DATA_PER_BATCH = 1024
  };
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpriteRenderer, 1, ezRTTIDefaultAllocator<ezSpriteRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSpriteRenderer::ezSpriteRenderer()
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Materials/SpriteMaterial.ezShader");
}

ezSpriteRenderer::~ezSpriteRenderer() {}

void ezSpriteRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types) const
{
  types.PushBack(ezGetStaticRTTI<ezSpriteRenderData>());
}

void ezSpriteRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& categories) const
{
  categories.PushBack(ezDefaultRenderDataCategories::LitMasked);
  categories.PushBack(ezDefaultRenderDataCategories::LitTransparent);
  categories.PushBack(ezDefaultRenderDataCategories::SimpleOpaque);
  categories.PushBack(ezDefaultRenderDataCategories::SimpleTransparent);
  categories.PushBack(ezDefaultRenderDataCategories::Selection);
}

void ezSpriteRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;

  const ezSpriteRenderData* pRenderData = batch.GetFirstData<ezSpriteRenderData>();

  ezGALBufferHandle hSpriteData = CreateSpriteDataBuffer();
  EZ_SCOPE_EXIT(DeleteSpriteDataBuffer(hSpriteData));

  pContext->BindShader(m_hShader);
  pContext->BindBuffer("spriteData", pDevice->GetDefaultResourceView(hSpriteData));
  pContext->BindTexture2D("SpriteTexture", pRenderData->m_hTexture);

  pContext->SetShaderPermutationVariable("BLEND_MODE", ezSpriteBlendMode::GetPermutationValue(pRenderData->m_BlendMode));

  ezUInt32 uiStartIndex = 0;
  while (uiStartIndex < batch.GetCount())
  {
    const ezUInt32 uiCount = ezMath::Min(batch.GetCount() - uiStartIndex, (ezUInt32)MAX_SPRITE_DATA_PER_BATCH);

    FillSpriteData(batch, uiStartIndex, uiCount);
    if (m_spriteData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
    {
      pContext->GetCommandEncoder()->UpdateBuffer(hSpriteData, 0, m_spriteData.GetByteArrayPtr());

      pContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, uiCount * 2);
      pContext->DrawMeshBuffer().IgnoreResult();
    }

    uiStartIndex += uiCount;
  }
}

ezGALBufferHandle ezSpriteRenderer::CreateSpriteDataBuffer() const
{
  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(SpriteData);
  desc.m_uiTotalSize = desc.m_uiStructSize * MAX_SPRITE_DATA_PER_BATCH;
  desc.m_BufferType = ezGALBufferType::Generic;
  desc.m_bUseAsStructuredBuffer = true;
  desc.m_bAllowShaderResourceView = true;
  desc.m_ResourceAccess.m_bImmutable = false;

  return ezGPUResourcePool::GetDefaultInstance()->GetBuffer(desc);
}

void ezSpriteRenderer::DeleteSpriteDataBuffer(ezGALBufferHandle hBuffer) const
{
  ezGPUResourcePool::GetDefaultInstance()->ReturnBuffer(hBuffer);
}

void ezSpriteRenderer::FillSpriteData(const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32 uiCount) const
{
  m_spriteData.Clear();
  m_spriteData.Reserve(uiCount);

  for (auto it = batch.GetIterator<ezSpriteRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    const ezSpriteRenderData* pRenderData = it;

    auto& spriteData = m_spriteData.ExpandAndGetRef();

    spriteData.m_worldSpacePosition = pRenderData->m_GlobalTransform.m_vPosition;
    spriteData.m_size = pRenderData->m_fSize;
    spriteData.m_maxScreenSize = pRenderData->m_fMaxScreenSize;
    spriteData.m_aspectRatio = pRenderData->m_fAspectRatio;
    spriteData.m_colorRG = ezShaderUtils::Float2ToRG16F(ezVec2(pRenderData->m_color.r, pRenderData->m_color.g));
    spriteData.m_colorBA = ezShaderUtils::Float2ToRG16F(ezVec2(pRenderData->m_color.b, pRenderData->m_color.a));
    spriteData.m_texCoordScale = ezShaderUtils::Float2ToRG16F(pRenderData->m_texCoordScale);
    spriteData.m_texCoordOffset = ezShaderUtils::Float2ToRG16F(pRenderData->m_texCoordOffset);
    spriteData.m_gameObjectID = pRenderData->m_uiUniqueID;
    spriteData.m_reserved = 0;
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SpriteRenderer);
