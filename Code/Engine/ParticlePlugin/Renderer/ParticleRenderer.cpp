#include <ParticlePlugin/PCH.h>
#include <ParticlePlugin/Renderer/ParticleRenderer.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Renderer/ParticleRenderData.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <CoreUtils/DataProcessing/Stream/Stream.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezParticleRenderer, 1, ezRTTIDefaultAllocator<ezParticleRenderer>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezParticleRenderData::ezParticleRenderData()
{
  m_pParticleSystem = nullptr;
}

ezParticleRenderer::ezParticleRenderer()
{
}

ezParticleRenderer::~ezParticleRenderer()
{
}

void ezParticleRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezParticleRenderData>());
}

void ezParticleRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezRenderContext* pContext = renderViewContext.m_pRenderContext;
  ezGALContext* pGALContext = pContext->GetGALContext();

  //pContext->BindMaterial(hMaterial);
  //pContext->BindMeshBuffer(pMesh->GetMeshBuffer());

  for (auto it = batch.GetIterator<ezParticleRenderData>(0, batch.GetCount()); it.IsValid(); ++it)
  {
    const ezParticleRenderData* pRenderData = it;

    const ezUInt64 uiNumParticles = pRenderData->m_pParticleSystem->GetNumActiveParticles();
    const ezVec3* pPosition = pRenderData->m_pParticleSystem->GetStreamPosition()->GetData<ezVec3>();
    const ezColor* pColor = pRenderData->m_pParticleSystem->GetStreamColor()->GetData<ezColor>();

    for (ezUInt64 i = 0; i < uiNumParticles; ++i)
    {
      const ezVec3 vCenter = /*pRenderData->m_GlobalTransform **/ pPosition[i];

      ezBoundingBox box;
      box.SetInvalid();
      box.ExpandToInclude(vCenter - ezVec3(-0.1f));
      box.ExpandToInclude(vCenter - ezVec3(+0.1f));

      ezDebugRenderer::DrawLineBox(renderViewContext.m_uiWorldIndex, box, pColor[i]);
    }

    // now it may be deleted
    pRenderData->m_pParticleSystem->DecreaseRefCount();
  }
}


