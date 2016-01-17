#include <RendererCore/PCH.h>
#include <RendererCore/Lights/LightGatheringRenderer.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/ConstantBuffers/ConstantBufferResource.h>
#include <Core/ResourceManager/ResourceManager.h>

#include <RendererCore/../../../Shared/Data/Shaders/Common/ObjectConstants.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLightGatheringRenderer, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

void ezLightGatheringRenderer::GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& types)
{
  types.PushBack(ezGetStaticRTTI<ezPointLightRenderData>());
  types.PushBack(ezGetStaticRTTI<ezSpotLightRenderData>());
  types.PushBack(ezGetStaticRTTI<ezDirectionalLightRenderData>());
}

ezUInt32 ezLightGatheringRenderer::Render(const ezRenderViewContext& renderViewContext, ezRenderPipelinePass* pPass, const ezArrayPtr<const ezRenderData* const>& renderData)
{
  // TODO: Setup necessary buffers to hold the light information

  ezUInt32 uiDataRendered = 0;
  while (uiDataRendered < renderData.GetCount() && renderData[uiDataRendered]->IsInstanceOf<ezLightRenderData>())
  {
    const ezLightRenderData* current = static_cast<const ezLightRenderData*>(renderData[uiDataRendered]);
    
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

    ++uiDataRendered;
  }

  return uiDataRendered;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_LightGatheringRenderer);

