#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Components/SpriteComponent.h>
#include <RendererCore/Components/SpriteRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <Shaders/Materials/SpriteData.h>
static_assert(sizeof(ezPerSpriteData) == 48);

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpriteRenderer, 1, ezRTTIDefaultAllocator<ezSpriteRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSpriteRenderer::ezSpriteRenderer()
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Materials/SpriteMaterial.ezShader");
}

ezSpriteRenderer::~ezSpriteRenderer() = default;

void ezSpriteRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezSpriteRenderData>());
}

void ezSpriteRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(ezDefaultRenderDataCategories::SimpleOpaque);
  ref_categories.PushBack(ezDefaultRenderDataCategories::SimpleTransparent);
  ref_categories.PushBack(ezDefaultRenderDataCategories::Selection);
}

void ezSpriteRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;

  const ezSpriteRenderData* pRenderData = batch.GetFirstData<ezSpriteRenderData>();

  const ezUInt32 uiBufferSize = ezMath::RoundUp(batch.GetCount(), 128u);
  ezGALBufferHandle hSpriteData = CreateSpriteDataBuffer(uiBufferSize);
  EZ_SCOPE_EXIT(DeleteSpriteDataBuffer(hSpriteData));

  pContext->BindShader(m_hShader);
  ezBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
  bindGroupRenderPass.BindBuffer("spriteData", hSpriteData);
  bindGroupRenderPass.BindTexture("SpriteTexture", pRenderData->m_hTexture);

  pContext->SetShaderPermutationVariable("BLEND_MODE", ezSpriteBlendMode::GetPermutationValue(pRenderData->m_BlendMode));
  pContext->SetShaderPermutationVariable("SHAPE_ICON", pRenderData->m_BlendMode == ezSpriteBlendMode::ShapeIcon ? ezMakeHashedString("TRUE") : ezMakeHashedString("FALSE"));

  FillSpriteData(batch);

  if (m_SpriteData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
  {
    pContext->GetCommandEncoder()->UpdateBuffer(hSpriteData, 0, m_SpriteData.GetByteArrayPtr(), ezGALUpdateMode::AheadOfTime);

    pContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, m_SpriteData.GetCount() * 2);
    pContext->DrawMeshBuffer().IgnoreResult();
  }
}

ezGALBufferHandle ezSpriteRenderer::CreateSpriteDataBuffer(ezUInt32 uiBufferSize) const
{
  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(ezPerSpriteData);
  desc.m_uiTotalSize = desc.m_uiStructSize * uiBufferSize;
  desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::Transient;
  desc.m_ResourceAccess.m_bImmutable = false;

  return ezGPUResourcePool::GetDefaultInstance()->GetBuffer(desc);
}

void ezSpriteRenderer::DeleteSpriteDataBuffer(ezGALBufferHandle hBuffer) const
{
  ezGPUResourcePool::GetDefaultInstance()->ReturnBuffer(hBuffer);
}

void ezSpriteRenderer::FillSpriteData(const ezRenderDataBatch& batch) const
{
  m_SpriteData.Clear();
  m_SpriteData.Reserve(batch.GetCount());

  for (auto it = batch.GetIterator<ezSpriteRenderData>(); it.IsValid(); ++it)
  {
    const ezSpriteRenderData* pRenderData = it;

    auto& spriteData = m_SpriteData.ExpandAndGetRef();

    spriteData.WorldSpacePosition = pRenderData->m_GlobalTransform.m_vPosition;
    spriteData.Size = pRenderData->m_fSize;
    spriteData.MaxScreenSize = pRenderData->m_fMaxScreenSize;
    spriteData.AspectRatio = pRenderData->m_fAspectRatio;
    spriteData.ColorRG = ezShaderUtils::Float2ToRG16F(ezVec2(pRenderData->m_color.r, pRenderData->m_color.g));
    spriteData.ColorBA = ezShaderUtils::Float2ToRG16F(ezVec2(pRenderData->m_color.b, pRenderData->m_color.a));
    spriteData.TexCoordScale = ezShaderUtils::Float2ToRG16F(pRenderData->m_texCoordScale);
    spriteData.TexCoordOffset = ezShaderUtils::Float2ToRG16F(pRenderData->m_texCoordOffset);
    spriteData.GameObjectID = pRenderData->m_uiUniqueID;
    spriteData.Reserved = 0;
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SpriteRenderer);
