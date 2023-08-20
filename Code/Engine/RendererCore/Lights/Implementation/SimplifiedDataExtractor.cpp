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

ezSimplifiedDataExtractor::~ezSimplifiedDataExtractor() = default;

void ezSimplifiedDataExtractor::PostSortAndBatch(
  const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData)
{
  ezSimplifiedDataCPU* pData = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), ezSimplifiedDataCPU);

  pData->m_uiSkyIrradianceIndex = view.GetWorld()->GetIndex();
  pData->m_cameraUsageHint = view.GetCameraUsageHint();

  ref_extractedRenderData.AddFrameData(pData);
}

ezResult ezSimplifiedDataExtractor::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return EZ_SUCCESS;
}

ezResult ezSimplifiedDataExtractor::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  // const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  return EZ_SUCCESS;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SimplifiedDataExtractor);
