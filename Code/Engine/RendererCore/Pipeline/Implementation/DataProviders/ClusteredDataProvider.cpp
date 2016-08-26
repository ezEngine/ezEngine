#include <RendererCore/PCH.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/DataProviders/ClusteredDataProvider.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>

#include <Foundation/Math/Float16.h>

EZ_CHECK_AT_COMPILETIME(sizeof(PerLightData) == 48);

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezClusteredDataProvider, 1, ezRTTIDefaultAllocator<ezClusteredDataProvider>)
{
}
EZ_END_DYNAMIC_REFLECTED_TYPE

namespace
{
  ezUInt32 Float3ToRGB10(ezVec3 value)
  {
    ezVec3 unsignedValue = value * 0.5f + ezVec3(0.5f);

    ezUInt32 r = ezMath::Clamp(static_cast<ezUInt32>(unsignedValue.x * 1023.0f + 0.5f), 0u, 1023u);
    ezUInt32 g = ezMath::Clamp(static_cast<ezUInt32>(unsignedValue.y * 1023.0f + 0.5f), 0u, 1023u);
    ezUInt32 b = ezMath::Clamp(static_cast<ezUInt32>(unsignedValue.z * 1023.0f + 0.5f), 0u, 1023u);

    return r | (g << 10) | (b << 20);
  }

  ezUInt32 Float2ToRG16F(ezVec2 value)
  {
    ezUInt32 r = ezFloat16(value.x).GetRawData();
    ezUInt32 g = ezFloat16(value.y).GetRawData();

    return r | (g << 16);
  }

  void FillLightData(PerLightData& perLightData, const ezLightRenderData* pLightRenderData)
  {
    ezMemoryUtils::ZeroFill(&perLightData);

    ezUInt32 uiType;

    if (auto pPointLightRenderData = ezDynamicCast<const ezPointLightRenderData*>(pLightRenderData))
    {
      uiType = LIGHT_TYPE_POINT;
      perLightData.position = pPointLightRenderData->m_GlobalTransform.m_vPosition;
      perLightData.invSqrAttRadius = 1.0f / (pPointLightRenderData->m_fRange * pPointLightRenderData->m_fRange);
    }
    else if (auto pSpotLightRenderData = ezDynamicCast<const ezSpotLightRenderData*>(pLightRenderData))
    {
      uiType = LIGHT_TYPE_SPOT;
      perLightData.direction = Float3ToRGB10(-pSpotLightRenderData->m_GlobalTransform.m_Rotation.GetColumn(0));
      perLightData.position = pSpotLightRenderData->m_GlobalTransform.m_vPosition;
      perLightData.invSqrAttRadius = 1.0f / (pSpotLightRenderData->m_fRange * pSpotLightRenderData->m_fRange);

      const float fCosInner = ezMath::Cos(pSpotLightRenderData->m_InnerSpotAngle * 0.5f);
      const float fCosOuter = ezMath::Cos(pSpotLightRenderData->m_OuterSpotAngle * 0.5f);
      const float fSpotParamScale = 1.0f / ezMath::Max(0.001f, (fCosInner - fCosOuter));
      const float fSpotParamOffset = -fCosOuter * fSpotParamScale;
      perLightData.spotParams = Float2ToRG16F(ezVec2(fSpotParamScale, fSpotParamOffset));
    }
    else if (auto pDirLightRenderData = ezDynamicCast<const ezDirectionalLightRenderData*>(pLightRenderData))
    {
      uiType = LIGHT_TYPE_DIR;
      perLightData.direction = Float3ToRGB10(-pDirLightRenderData->m_GlobalTransform.m_Rotation.GetColumn(0));
    }
    else
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }

    ezColorLinearUB lightColor = pLightRenderData->m_LightColor;
    lightColor.a = uiType;

    perLightData.colorAndType = *reinterpret_cast<ezUInt32*>(&lightColor.r);
    perLightData.intensity = pLightRenderData->m_fIntensity;
  }

  enum
  {
    MAX_LIGHT_DATA = 1024
  };
}

ezClusteredData::ezClusteredData()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  {
    ezGALBufferCreationDescription desc;
    desc.m_uiStructSize = sizeof(PerLightData);
    desc.m_uiTotalSize = desc.m_uiStructSize * MAX_LIGHT_DATA;
    desc.m_BufferType = ezGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hLightDataBuffer = pDevice->CreateBuffer(desc);
  }

  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ClusteredDataConstants>();
}

ezClusteredData::~ezClusteredData()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  pDevice->DestroyBuffer(m_hLightDataBuffer);

  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void ezClusteredData::BindResources(ezRenderContext* pRenderContext)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    pRenderContext->BindBuffer((ezGALShaderStage::Enum)stage, "perLightData", pDevice->GetDefaultResourceView(m_hLightDataBuffer));
  }

  pRenderContext->BindConstantBuffer("ClusteredDataConstants", m_hConstantBuffer);
}

//////////////////////////////////////////////////////////////////////////

ezClusteredDataProvider::ezClusteredDataProvider()
{
  
}

ezClusteredDataProvider::~ezClusteredDataProvider()
{
 
}

void* ezClusteredDataProvider::UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData)
{
  {
    // Collect light data
    m_Data.m_LightData.Clear();

    auto batchList = extractedData.GetRenderDataBatchesWithCategory(ezDefaultRenderDataCategories::Light);
    const ezUInt32 uiBatchCount = batchList.GetBatchCount();
    for (ezUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const ezRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<ezLightRenderData>(); it.IsValid(); ++it)
      {
        FillLightData(m_Data.m_LightData.ExpandAndGetRef(), it);
      }
    }

    // Update buffer
    if (!m_Data.m_LightData.IsEmpty())
    {
      renderViewContext.m_pRenderContext->GetGALContext()->UpdateBuffer(m_Data.m_hLightDataBuffer, 0, m_Data.m_LightData.GetByteArrayPtr());
    }
  }  

  ClusteredDataConstants* pConstants = renderViewContext.m_pRenderContext->GetConstantBufferData<ClusteredDataConstants>(m_Data.m_hConstantBuffer);
  pConstants->NumLights = m_Data.m_LightData.GetCount();

  return &m_Data;
}
