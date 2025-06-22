#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/SimplifiedDataExtractor.h>
#include <RendererCore/Lights/SimplifiedDataProvider.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightDataSimplified.h>
EZ_DEFINE_AS_POD_TYPE(ezSimplifiedDataConstants);

ezSimplifiedDataGPU::ezSimplifiedDataGPU()
{
  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezSimplifiedDataConstants>();
}

ezSimplifiedDataGPU::~ezSimplifiedDataGPU()
{
  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void ezSimplifiedDataGPU::BindResources(ezRenderContext* pRenderContext)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezBindGroupBuilder& bindGroup = pRenderContext->GetBindGroup();
  bindGroup.BindTexture("ReflectionSpecularTexture", ezReflectionPool::GetReflectionSpecularTexture(m_uiSkyIrradianceIndex, m_cameraUsageHint));
  bindGroup.BindTexture("SkyIrradianceTexture", ezReflectionPool::GetSkyIrradianceTexture());

  bindGroup.BindBuffer("ezSimplifiedDataConstants", m_hConstantBuffer);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimplifiedDataProvider, 1, ezRTTIDefaultAllocator<ezSimplifiedDataProvider>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSimplifiedDataProvider::ezSimplifiedDataProvider() = default;

ezSimplifiedDataProvider::~ezSimplifiedDataProvider() = default;

void* ezSimplifiedDataProvider::UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData)
{
  ezGALCommandEncoder* pGALCommandEncoder = renderViewContext.m_pRenderContext->GetCommandEncoder();

  EZ_PROFILE_AND_MARKER(pGALCommandEncoder, "Update Clustered Data");

  if (auto pData = extractedData.GetFrameData<ezSimplifiedDataCPU>())
  {
    m_Data.m_uiSkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
    m_Data.m_cameraUsageHint = pData->m_cameraUsageHint;

    // Update Constants
    const ezRectFloat& viewport = renderViewContext.m_pViewData->m_ViewPortRect;

    ezSimplifiedDataConstants* pConstants =
      renderViewContext.m_pRenderContext->GetConstantBufferData<ezSimplifiedDataConstants>(m_Data.m_hConstantBuffer);

    pConstants->SkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
  }

  return &m_Data;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SimplifiedDataProvider);
