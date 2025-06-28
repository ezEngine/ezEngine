#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Components/LensFlareComponent.h>
#include <RendererCore/Components/LensFlareRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <Shaders/Materials/LensFlareData.h>
static_assert(sizeof(ezPerLensFlareData) == 48);

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLensFlareRenderer, 1, ezRTTIDefaultAllocator<ezLensFlareRenderer>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezLensFlareRenderer::ezLensFlareRenderer()
{
  m_hShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Materials/LensFlareMaterial.ezShader");
}

ezLensFlareRenderer::~ezLensFlareRenderer() = default;

void ezLensFlareRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(ezGetStaticRTTI<ezLensFlareRenderData>());
}

void ezLensFlareRenderer::GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(ezDefaultRenderDataCategories::LitTransparent);
}

void ezLensFlareRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;

  const ezLensFlareRenderData* pRenderData = batch.GetFirstData<ezLensFlareRenderData>();

  const ezUInt32 uiBufferSize = ezMath::RoundUp(batch.GetCount(), 128u);
  ezGALBufferHandle hLensFlareData = CreateLensFlareDataBuffer(uiBufferSize);
  EZ_SCOPE_EXIT(DeleteLensFlareDataBuffer(hLensFlareData));

  ezBindGroupBuilder& bindGroupRenderPass = ezRenderContext::GetDefaultInstance()->GetBindGroup(EZ_GAL_BIND_GROUP_RENDER_PASS);
  pContext->BindShader(m_hShader);
  bindGroupRenderPass.BindBuffer("lensFlareData", hLensFlareData);
  bindGroupRenderPass.BindTexture("LensFlareTexture", pRenderData->m_hTexture);

  FillLensFlareData(batch);

  if (m_LensFlareData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
  {
    pContext->GetCommandEncoder()->UpdateBuffer(hLensFlareData, 0, m_LensFlareData.GetByteArrayPtr(), ezGALUpdateMode::AheadOfTime);

    pContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, m_LensFlareData.GetCount() * 2);
    pContext->DrawMeshBuffer().IgnoreResult();
  }
}

ezGALBufferHandle ezLensFlareRenderer::CreateLensFlareDataBuffer(ezUInt32 uiBufferSize) const
{
  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(ezPerLensFlareData);
  desc.m_uiTotalSize = desc.m_uiStructSize * uiBufferSize;
  desc.m_BufferFlags = ezGALBufferUsageFlags::StructuredBuffer | ezGALBufferUsageFlags::ShaderResource | ezGALBufferUsageFlags::Transient;
  desc.m_ResourceAccess.m_bImmutable = false;

  return ezGPUResourcePool::GetDefaultInstance()->GetBuffer(desc);
}

void ezLensFlareRenderer::DeleteLensFlareDataBuffer(ezGALBufferHandle hBuffer) const
{
  ezGPUResourcePool::GetDefaultInstance()->ReturnBuffer(hBuffer);
}

void ezLensFlareRenderer::FillLensFlareData(const ezRenderDataBatch& batch) const
{
  m_LensFlareData.Clear();
  m_LensFlareData.Reserve(batch.GetCount());

  for (auto it = batch.GetIterator<ezLensFlareRenderData>(); it.IsValid(); ++it)
  {
    const ezLensFlareRenderData* pRenderData = it;

    auto& LensFlareData = m_LensFlareData.ExpandAndGetRef();
    LensFlareData.WorldSpacePosition = pRenderData->m_GlobalTransform.m_vPosition;
    LensFlareData.Size = pRenderData->m_fSize;
    LensFlareData.MaxScreenSize = pRenderData->m_fMaxScreenSize;
    LensFlareData.OcclusionRadius = pRenderData->m_fOcclusionSampleRadius;
    LensFlareData.OcclusionSpread = pRenderData->m_fOcclusionSampleSpread;
    LensFlareData.DepthOffset = pRenderData->m_fOcclusionDepthOffset;
    LensFlareData.AspectRatioAndShift = ezShaderUtils::Float2ToRG16F(ezVec2(pRenderData->m_fAspectRatio, pRenderData->m_fShiftToCenter));
    LensFlareData.ColorRG = ezShaderUtils::PackFloat16intoUint(pRenderData->m_Color.x, pRenderData->m_Color.y);
    LensFlareData.ColorBA = ezShaderUtils::PackFloat16intoUint(pRenderData->m_Color.z, pRenderData->m_Color.w);
    LensFlareData.Flags = (pRenderData->m_bInverseTonemap ? LENS_FLARE_INVERSE_TONEMAP : 0) |
                          (pRenderData->m_bGreyscaleTexture ? LENS_FLARE_GREYSCALE_TEXTURE : 0) |
                          (pRenderData->m_bApplyFog ? LENS_FLARE_APPLY_FOG : 0);
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_LensFlareRenderer);
