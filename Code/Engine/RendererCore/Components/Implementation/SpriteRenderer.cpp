#include <PCH.h>
#include <RendererCore/Components/SpriteComponent.h>
#include <RendererCore/Components/SpriteRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>

struct EZ_ALIGN_16(SpriteData)
{
  ezVec3 m_worldSpacePosition;
  ezUInt32 m_size;
  ezColorLinearUB m_color;
  ezUInt32 m_texCoordScale;
  ezUInt32 m_texCoordOffset;
  ezUInt32 m_gameObjectID;
};

EZ_CHECK_AT_COMPILETIME(sizeof(SpriteData) == 32);

namespace
{
  ///\todo find a better place for this function
  ezUInt32 Float2ToRG16F(ezVec2 value)
  {
    ezUInt32 r = ezFloat16(value.x).GetRawData();
    ezUInt32 g = ezFloat16(value.y).GetRawData();

    return r | (g << 16);
  }

  enum
  {
    MAX_SPRITE_DATA_PER_BATCH = 1024
  };
}


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpriteRenderer, 1, ezRTTIDefaultAllocator<ezSpriteRenderer>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSpriteRenderer::ezSpriteRenderer()
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Materials/SpriteMaterial.ezShader");
}

ezSpriteRenderer::~ezSpriteRenderer()
{
}

void ezSpriteRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezSpriteRenderData>());
}

void ezSpriteRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;
  ezGALContext* pGALContext = pContext->GetGALContext();

  const ezSpriteRenderData* pRenderData = batch.GetFirstData<ezSpriteRenderData>();

  ezGALBufferHandle hSpriteData = CreateSpriteDataBuffer();
  EZ_SCOPE_EXIT(DeleteSpriteDataBuffer(hSpriteData));

  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindBuffer(ezGALShaderStage::VertexShader, "spriteData", pDevice->GetDefaultResourceView(hSpriteData));
  renderViewContext.m_pRenderContext->BindTexture2D(ezGALShaderStage::PixelShader, "SpriteTexture", pRenderData->m_hTexture);

  ezUInt32 uiStartIndex = 0;
  while (uiStartIndex < batch.GetCount())
  {
    const ezUInt32 uiCount = ezMath::Min(batch.GetCount() - uiStartIndex, (ezUInt32)MAX_SPRITE_DATA_PER_BATCH);

    FillSpriteData(batch, uiStartIndex, uiCount);
    if (m_spriteData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
    {
      pGALContext->UpdateBuffer(hSpriteData, 0, m_spriteData.GetByteArrayPtr());

      renderViewContext.m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr,
        ezGALPrimitiveTopology::Triangles, uiCount * 2);
      renderViewContext.m_pRenderContext->DrawMeshBuffer();
    }

    uiStartIndex += uiCount;
  }
}

ezGALBufferHandle ezSpriteRenderer::CreateSpriteDataBuffer()
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

void ezSpriteRenderer::DeleteSpriteDataBuffer(ezGALBufferHandle hBuffer)
{
  ezGPUResourcePool::GetDefaultInstance()->ReturnBuffer(hBuffer);
}

void ezSpriteRenderer::FillSpriteData(const ezRenderDataBatch& batch, ezUInt32 uiStartIndex, ezUInt32 uiCount)
{
  m_spriteData.Clear();
  m_spriteData.Reserve(uiCount);

  for (auto it = batch.GetIterator<ezSpriteRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    const ezSpriteRenderData* pRenderData = it;

    auto& spriteData = m_spriteData.ExpandAndGetRef();

    spriteData.m_worldSpacePosition = pRenderData->m_GlobalTransform.m_vPosition;
    spriteData.m_size = Float2ToRG16F(ezVec2(pRenderData->m_fSize, pRenderData->m_fMaxScreenSize));
    spriteData.m_color = pRenderData->m_color;
    spriteData.m_texCoordScale = Float2ToRG16F(pRenderData->m_texCoordScale);
    spriteData.m_texCoordOffset = Float2ToRG16F(pRenderData->m_texCoordOffset);
    spriteData.m_gameObjectID = pRenderData->m_uiUniqueID;
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SpriteRenderer);

