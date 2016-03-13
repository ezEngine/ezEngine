#include <RendererCore/PCH.h>
#include <RendererCore/Lights/LightGatheringRenderer.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>
#include <Core/ResourceManager/ResourceManager.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/ObjectConstants.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLightGatheringRenderer, 1, ezRTTIDefaultAllocator<ezLightGatheringRenderer>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

void ezLightGatheringRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezPointLightRenderData>());
  types.PushBack(ezGetStaticRTTI<ezSpotLightRenderData>());
  types.PushBack(ezGetStaticRTTI<ezDirectionalLightRenderData>());
}

void ezLightGatheringRenderer::RenderBatch(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch)
{
  // TODO: Setup necessary buffers to hold the light information

  for (auto it = batch.GetIterator<ezLightRenderData>(); it.IsValid(); ++it)
  {
    const ezLightRenderData* current = it;
    
    // TODO: Fill buffers with light render data
    if (current->IsInstanceOf<ezPointLightRenderData>())
    {

    }
    else if (current->IsInstanceOf<ezSpotLightRenderData>())
    {

    }
    else if (current->IsInstanceOf<ezDirectionalLightRenderData>())
    {

    }
  }
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_LightGatheringRenderer);

