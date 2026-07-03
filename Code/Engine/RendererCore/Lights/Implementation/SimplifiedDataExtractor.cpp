#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Lights/Implementation/ClusteredDataUtils.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/SimplifiedDataExtractor.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererFoundation/Device/Device.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightDataSimplified.h>
EZ_DEFINE_AS_POD_TYPE(ezSimplifiedDataConstants);

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSimplifiedDataCPU, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSimplifiedDataCPU::ezSimplifiedDataCPU() = default;
ezSimplifiedDataCPU::~ezSimplifiedDataCPU() = default;

ezSimplifiedDataGPU::ezSimplifiedDataGPU()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezGALBufferCreationDescription desc;
  desc.m_uiStructSize = 0;
  desc.m_uiTotalSize = sizeof(ezSimplifiedDataConstants);
  desc.m_BufferFlags = ezGALBufferUsageFlags::ConstantBuffer;
  m_hConstantBuffer = pDevice->CreateBuffer(desc);
}

ezSimplifiedDataGPU::~ezSimplifiedDataGPU()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  pDevice->DestroyBuffer(m_hConstantBuffer);
}

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

  UpdateGpuData(view, pData);
  AddGpuData(view, ref_extractedRenderData);

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
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  return EZ_SUCCESS;
}

void ezSimplifiedDataExtractor::UpdateGpuData(const ezView& view, const ezSimplifiedDataCPU* pData)
{
  EZ_IGNORE_UNUSED(view);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  m_DataGPU.m_uiSkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;
  m_DataGPU.m_cameraUsageHint = pData->m_cameraUsageHint;

  ezSimplifiedDataConstants constants = {};
  constants.SkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;

  pDevice->UpdateBufferForNextFrame(m_DataGPU.m_hConstantBuffer, ezMakeByteArrayPtr(&constants, 1), 0);
}

void ezSimplifiedDataExtractor::AddGpuData(const ezView& view, ezExtractedRenderData& ref_extractedRenderData)
{
  const ezEnum<ezCameraUsageHint> cameraUsageHint = view.GetCameraUsageHint();
  // Reflection specular and sky irradiance textures
  {
    ezGALTextureHandle hReflSpec = ezReflectionPool::GetReflectionSpecularTexture(view.GetWorld()->GetIndex(), cameraUsageHint);
    ref_extractedRenderData.AddViewDependency(hReflSpec, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::Auto);
    ref_extractedRenderData.AddTextureBinding(ezTempHashedString("ReflectionSpecularTexture"), hReflSpec);

    ezGALTextureHandle hSkyIrradiance = ezReflectionPool::GetSkyIrradianceTexture();
    ref_extractedRenderData.AddViewDependency(hSkyIrradiance, ezGALResourceState::ShaderResource, ezGALShaderStageFlags::Auto);
    ref_extractedRenderData.AddTextureBinding(ezTempHashedString("SkyIrradianceTexture"), hSkyIrradiance);
  }

  ref_extractedRenderData.AddBufferBinding("ezSimplifiedDataConstants", m_DataGPU.m_hConstantBuffer);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SimplifiedDataExtractor);
