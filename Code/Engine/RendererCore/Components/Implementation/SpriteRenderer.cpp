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

float ezSpriteRenderer::s_fShapeIconScale = 1.0f;
float ezSpriteRenderer::s_fShapeIconFadeDistance = 100.0f;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpriteRenderer, 1, ezRTTIDefaultAllocator<ezSpriteRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSpriteRenderer::ezSpriteRenderer()
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Materials/SpriteMaterial.ezShader");
}

ezSpriteRenderer::~ezSpriteRenderer() = default;

void ezSpriteRenderer::GetSupportedRenderDataTypes(ezDynamicArray<const ezRTTI*>& out_types) const
{
  out_types.PushBack(ezGetStaticRTTI<ezSpriteRenderData>());
}

void ezSpriteRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;

  const ezSpriteRenderData* pRenderData = batch.GetFirstData<ezSpriteRenderData>();

  const ezUInt32 uiBufferSize = ezMath::RoundUp(batch.GetDataCount(), 128u);
  ezGALBufferHandle hSpriteData = CreateSpriteDataBuffer(uiBufferSize);
  EZ_SCOPE_EXIT(DeleteSpriteDataBuffer(hSpriteData));

  pContext->BindShader(m_hShader);
  ezBindGroupBuilder& bindGroupRenderPass = renderViewContext.m_pRenderContext->GetBindGroup(EZ_GAL_BIND_GROUP_DRAW_CALL);
  bindGroupRenderPass.BindBuffer("spriteData", hSpriteData);
  bindGroupRenderPass.BindTexture("SpriteTexture", pRenderData->m_hTexture);

  pContext->SetShaderPermutationVariable("BLEND_MODE", ezSpriteBlendMode::GetPermutationValue(pRenderData->m_BlendMode));
  pContext->SetShaderPermutationVariable("SHAPE_ICON", pRenderData->m_BlendMode == ezSpriteBlendMode::ShapeIcon ? ezMakeHashedString("TRUE") : ezMakeHashedString("FALSE"));

  FillSpriteData(batch);

  if (m_SpriteData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
  {
    pContext->GetCommandEncoder()->UpdateBuffer(hSpriteData, 0, m_SpriteData.GetByteArrayPtr(), ezGALUpdateMode::AheadOfTime);

    pContext->BindNullMeshBuffer(ezGALPrimitiveTopology::Triangles, m_SpriteData.GetCount() * 2);
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
  m_SpriteData.Reserve(batch.GetDataCount());

  for (auto it = batch.GetIterator<ezSpriteRenderData>(); it.IsValid(); ++it)
  {
    const ezSpriteRenderData* pRenderData = it;

    auto& spriteData = m_SpriteData.ExpandAndGetRef();

    spriteData.WorldSpacePosition = pRenderData->m_vGlobalPosition;
    spriteData.Size = pRenderData->m_fSize;
    spriteData.MaxScreenSize = pRenderData->m_fMaxScreenSize;
    spriteData.AspectRatio = pRenderData->m_fAspectRatio;
    spriteData.ColorRG = ezShaderUtils::PackFloat16intoUint(pRenderData->m_color.r, pRenderData->m_color.g);
    spriteData.ColorBA = ezShaderUtils::PackFloat16intoUint(pRenderData->m_color.b, pRenderData->m_color.a);
    spriteData.TexCoordScale = ezShaderUtils::PackFloat16intoUint(pRenderData->m_texCoordScale.x, pRenderData->m_texCoordScale.y);
    spriteData.TexCoordOffset = ezShaderUtils::PackFloat16intoUint(pRenderData->m_texCoordOffset.x, pRenderData->m_texCoordOffset.y);
    spriteData.GameObjectID = pRenderData->m_uiUniqueID;
    spriteData.ShapeIconFadeDistance = s_fShapeIconFadeDistance;
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SpriteRenderer);
