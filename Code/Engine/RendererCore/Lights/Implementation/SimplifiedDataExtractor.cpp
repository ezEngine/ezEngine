#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ClusteredDataUtils.h>
#include <RendererCore/Lights/SimplifiedDataExtractor.h>
#include <RendererCore/Pipeline/View.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimplifiedDataCPU, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSimplifiedDataCPU::ezSimplifiedDataCPU() = default;
ezSimplifiedDataCPU::~ezSimplifiedDataCPU() = default;

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimplifiedDataExtractor, 1, ezRTTIDefaultAllocator<ezSimplifiedDataExtractor>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSimplifiedDataExtractor::ezSimplifiedDataExtractor(const char* szName)
  : ezExtractor(szName)
{
  m_DependsOn.PushBack(ezMakeHashedString("ezVisibleObjectsExtractor"));
}

ezSimplifiedDataExtractor::~ezSimplifiedDataExtractor() {}

void ezSimplifiedDataExtractor::PostSortAndBatch(
  const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& extractedRenderData)
{
  const ezCamera* pCamera = view.GetCullingCamera();
  const float fAspectRatio = view.GetViewport().width / view.GetViewport().height;

  ezSimplifiedDataCPU* pData = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), ezSimplifiedDataCPU);

  pData->m_uiSkyIrradianceIndex = view.GetWorld()->GetIndex();

  extractedRenderData.AddFrameData(pData);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SimplifiedDataExtractor);
