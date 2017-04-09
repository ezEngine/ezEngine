#include <PCH.h>
#include <RendererCore/Pipeline/DataProviders/ClusteredDataProvider.h>
#include <RendererCore/Pipeline/Implementation/DataProviders/ClusteredDataUtils.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <Foundation/Profiling/Profiling.h>

ezClusteredData::ezClusteredData()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezGALBufferCreationDescription desc;

  {
    desc.m_uiStructSize = sizeof(ezPerLightData);
    desc.m_uiTotalSize = desc.m_uiStructSize * MAX_LIGHT_DATA;
    desc.m_BufferType = ezGALBufferType::Generic;
    desc.m_bUseAsStructuredBuffer = true;
    desc.m_bAllowShaderResourceView = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hLightDataBuffer = pDevice->CreateBuffer(desc);
  }

  {
    desc.m_uiStructSize = sizeof(ezPerClusterData);
    desc.m_uiTotalSize = desc.m_uiStructSize * NUM_CLUSTERS;

    m_hClusterDataBuffer = pDevice->CreateBuffer(desc);
  }

  {
    desc.m_uiStructSize = sizeof(ezUInt32);
    desc.m_uiTotalSize = desc.m_uiStructSize * MAX_LIGHTS_PER_CLUSTER * NUM_CLUSTERS;

    m_hClusterItemListBuffer = pDevice->CreateBuffer(desc);
  }

  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezClusteredDataConstants>();

  m_ClusterData.SetCountUninitialized(NUM_CLUSTERS);
}

ezClusteredData::~ezClusteredData()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  pDevice->DestroyBuffer(m_hLightDataBuffer);
  pDevice->DestroyBuffer(m_hClusterDataBuffer);
  pDevice->DestroyBuffer(m_hClusterItemListBuffer);

  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void ezClusteredData::BindResources(ezRenderContext* pRenderContext)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  for (ezUInt32 stage = 0; stage < ezGALShaderStage::ENUM_COUNT; ++stage)
  {
    pRenderContext->BindBuffer((ezGALShaderStage::Enum)stage, "perLightData", pDevice->GetDefaultResourceView(m_hLightDataBuffer));
    pRenderContext->BindBuffer((ezGALShaderStage::Enum)stage, "perClusterData", pDevice->GetDefaultResourceView(m_hClusterDataBuffer));
    pRenderContext->BindBuffer((ezGALShaderStage::Enum)stage, "clusterItemList", pDevice->GetDefaultResourceView(m_hClusterItemListBuffer));
  }

  pRenderContext->BindConstantBuffer("ezClusteredDataConstants", m_hConstantBuffer);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezClusteredDataProvider, 1, ezRTTIDefaultAllocator<ezClusteredDataProvider>)
{
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezClusteredDataProvider::ezClusteredDataProvider()
{
  m_TempClusters.SetCountUninitialized(NUM_CLUSTERS);
}

ezClusteredDataProvider::~ezClusteredDataProvider()
{

}

void* ezClusteredDataProvider::UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData)
{
  EZ_PROFILE("Clustered Data Update");

  ezColor ambientTopColor = ezColor::Black;
  ezColor ambientBottomColor = ezColor::Black;

  {
    // Collect and rasterize light data
    m_Data.m_LightData.Clear();
    ezMemoryUtils::ZeroFill(m_TempClusters.GetData(), NUM_CLUSTERS);

    ezSimdMat4f viewProjectionMatrix = ezSimdConversion::ToMat4(renderViewContext.m_pViewData->m_ViewProjectionMatrix);
    ezVec3 cameraPosition = renderViewContext.m_pCamera->GetPosition();

    auto batchList = extractedData.GetRenderDataBatchesWithCategory(ezDefaultRenderDataCategories::Light);
    const ezUInt32 uiBatchCount = batchList.GetBatchCount();
    for (ezUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const ezRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<ezRenderData>(); it.IsValid(); ++it)
      {
        const ezUInt32 uiLightIndex = m_Data.m_LightData.GetCount();

        if (uiLightIndex == ezClusteredData::MAX_LIGHT_DATA)
        {
          ezLog::Warning("Maximum number of lights reached ({0}). Further lights will be discarded.", ezClusteredData::MAX_LIGHT_DATA);
          break;
        }

        if (auto pPointLightRenderData = ezDynamicCast<const ezPointLightRenderData*>(it))
        {
          FillPointLightData(m_Data.m_LightData.ExpandAndGetRef(), pPointLightRenderData);

          RasterizePointLight(pPointLightRenderData, uiLightIndex, viewProjectionMatrix, cameraPosition, m_TempClusters.GetArrayPtr());
        }
        else if (auto pSpotLightRenderData = ezDynamicCast<const ezSpotLightRenderData*>(it))
        {
          FillSpotLightData(m_Data.m_LightData.ExpandAndGetRef(), pSpotLightRenderData);

          RasterizeSpotLight(pSpotLightRenderData, uiLightIndex, viewProjectionMatrix, cameraPosition, m_TempClusters.GetArrayPtr());
        }
        else if (auto pDirLightRenderData = ezDynamicCast<const ezDirectionalLightRenderData*>(it))
        {
          FillDirLightData(m_Data.m_LightData.ExpandAndGetRef(), pDirLightRenderData);

          RasterizeDirLight(pDirLightRenderData, uiLightIndex, m_TempClusters.GetArrayPtr());
        }
        else if (auto pAmbientLightRenderData = ezDynamicCast<const ezAmbientLightRenderData*>(it))
        {
          ambientTopColor = pAmbientLightRenderData->m_TopColor;
          ambientBottomColor = pAmbientLightRenderData->m_BottomColor;
        }
        else
        {
          EZ_ASSERT_NOT_IMPLEMENTED;
        }
      }
    }

    FillItemListAndClusterData();

    ezGALContext* pGALContext = renderViewContext.m_pRenderContext->GetGALContext();

    // Update buffer
    if (!m_Data.m_LightData.IsEmpty())
    {
      pGALContext->UpdateBuffer(m_Data.m_hLightDataBuffer, 0, m_Data.m_LightData.GetByteArrayPtr());
      pGALContext->UpdateBuffer(m_Data.m_hClusterItemListBuffer, 0, m_Data.m_ClusterItemList.GetByteArrayPtr());
    }

    pGALContext->UpdateBuffer(m_Data.m_hClusterDataBuffer, 0, m_Data.m_ClusterData.GetByteArrayPtr());
  }

  const ezRectFloat& viewport = renderViewContext.m_pViewData->m_ViewPortRect;

  ezClusteredDataConstants* pConstants = renderViewContext.m_pRenderContext->GetConstantBufferData<ezClusteredDataConstants>(m_Data.m_hConstantBuffer);
  pConstants->DepthSliceScale = s_fDepthSliceScale;
  pConstants->DepthSliceBias = s_fDepthSliceBias;
  pConstants->InvTileSize = ezVec2(NUM_CLUSTERS_X / viewport.width, NUM_CLUSTERS_Y / viewport.height);
  pConstants->NumLights = m_Data.m_LightData.GetCount();
  pConstants->AmbientTopColor = ambientTopColor;
  pConstants->AmbientBottomColor = ambientBottomColor;

  return &m_Data;
}

void ezClusteredDataProvider::FillItemListAndClusterData()
{
  m_Data.m_ClusterItemList.Clear();

  const ezUInt32 uiNumLights = m_Data.m_LightData.GetCount();
  const ezUInt32 uiMaxBlockIndex = (uiNumLights + 31) / 32;

  for (ezUInt32 i = 0; i < NUM_CLUSTERS; ++i)
  {
    ezUInt32 uiOffset = m_Data.m_ClusterItemList.GetCount();
    ezUInt32 uiCount = 0;

    auto& tempCluster = m_TempClusters[i];
    for (ezUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxBlockIndex; ++uiBlockIndex)
    {
      ezUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

      while (mask > 0)
      {
        ezUInt32 uiLightIndex = ezMath::FirstBitLow(mask);
        mask &= ~(1 << uiLightIndex);

        m_Data.m_ClusterItemList.PushBack(uiLightIndex);
        ++uiCount;
      }
    }

    auto& clusterData = m_Data.m_ClusterData[i];
    clusterData.offset = uiOffset;
    clusterData.counts = uiCount;
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_DataProviders_ClusteredDataProvider);

