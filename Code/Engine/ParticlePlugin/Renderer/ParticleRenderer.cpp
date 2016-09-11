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

    EZ_LOCK(pRenderData->m_pParticleSystem->m_Mutex);

    if (pRenderData->m_pParticleSystem->Render(renderViewContext, pPass))
      continue;

    const ezUInt64 uiNumParticles = pRenderData->m_pParticleSystem->GetNumActiveParticles();

    const ezProcessingStream* pStreamPosition = pRenderData->m_pParticleSystem->QueryStream("Position", ezProcessingStream::DataType::Float3);
    const ezProcessingStream* pStreamColor = pRenderData->m_pParticleSystem->QueryStream("Color", ezProcessingStream::DataType::Float4);
    const ezProcessingStream* pStreamSize = pRenderData->m_pParticleSystem->QueryStream("Size", ezProcessingStream::DataType::Float);

    if (pStreamPosition == nullptr || pStreamPosition->GetData<ezVec3>() == nullptr)
      continue;

    const ezVec3* pPosition = pStreamPosition->GetData<ezVec3>();
    const ezColor* pColor = pStreamColor ? pStreamColor->GetData<ezColor>() : nullptr;
    const float* pSize = pStreamSize ? pStreamSize->GetData<float>() : nullptr;

    for (ezUInt64 i = 0; i < uiNumParticles; ++i)
    {
      const ezVec3 vCenter = pRenderData->m_GlobalTransform * pPosition[i];

      ezColor col = ezColor::White;
      float size = 0.1f;

      if (pColor)
        col = pColor[i];
      if (pSize)
        size = pSize[i];

      ezBoundingBox box;
      box.SetInvalid();
      box.ExpandToInclude(vCenter - ezVec3(+size));
      box.ExpandToInclude(vCenter - ezVec3(-size));

      ezDebugRenderer::DrawLineBox(renderViewContext.m_uiWorldIndex, box, col);
    }

    // now it may be deleted
    pRenderData->m_pParticleSystem->DecreaseRefCount();
  }
}


