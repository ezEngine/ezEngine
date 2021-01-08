#include <RendererCorePCH.h>

#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Lights/ClusteredDataExtractor.h>
#include <RendererCore/Lights/ClusteredDataProvider.h>
#include <RendererCore/Lights/Implementation/ClusteredDataUtils.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <RendererFoundation/Profiling/Profiling.h>

ezClusteredDataGPU::ezClusteredDataGPU()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  {
    ezGALBufferCreationDescription desc;

    {
      desc.m_uiStructSize = sizeof(ezPerLightData);
      desc.m_uiTotalSize = desc.m_uiStructSize * ezClusteredDataCPU::MAX_LIGHT_DATA;
      desc.m_BufferType = ezGALBufferType::Generic;
      desc.m_bUseAsStructuredBuffer = true;
      desc.m_bAllowShaderResourceView = true;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_hLightDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiStructSize = sizeof(ezPerDecalData);
      desc.m_uiTotalSize = desc.m_uiStructSize * ezClusteredDataCPU::MAX_DECAL_DATA;
      desc.m_BufferType = ezGALBufferType::Generic;
      desc.m_bUseAsStructuredBuffer = true;
      desc.m_bAllowShaderResourceView = true;
      desc.m_ResourceAccess.m_bImmutable = false;

      m_hDecalDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiStructSize = sizeof(ezPerClusterData);
      desc.m_uiTotalSize = desc.m_uiStructSize * NUM_CLUSTERS;

      m_hClusterDataBuffer = pDevice->CreateBuffer(desc);
    }

    {
      desc.m_uiStructSize = sizeof(ezUInt32);
      desc.m_uiTotalSize = desc.m_uiStructSize * ezClusteredDataCPU::MAX_ITEMS_PER_CLUSTER * NUM_CLUSTERS;

      m_hClusterItemBuffer = pDevice->CreateBuffer(desc);
    }
  }

  m_hConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezClusteredDataConstants>();

  {
    ezGALSamplerStateCreationDescription desc;
    desc.m_AddressU = ezImageAddressMode::Clamp;
    desc.m_AddressV = ezImageAddressMode::Clamp;
    desc.m_AddressW = ezImageAddressMode::Clamp;
    desc.m_SampleCompareFunc = ezGALCompareFunc::Less;

    m_hShadowSampler = pDevice->CreateSamplerState(desc);
  }

  m_hDecalAtlas = ezDecalAtlasResource::GetDecalAtlasResource();

  {
    ezGALSamplerStateCreationDescription desc;
    desc.m_AddressU = ezImageAddressMode::Clamp;
    desc.m_AddressV = ezImageAddressMode::Clamp;
    desc.m_AddressW = ezImageAddressMode::Clamp;

    ezTextureUtils::ConfigureSampler(ezTextureFilterSetting::DefaultQuality, desc);
    desc.m_uiMaxAnisotropy = ezMath::Min(desc.m_uiMaxAnisotropy, 4u);

    m_hDecalAtlasSampler = pDevice->CreateSamplerState(desc);
  }
}

ezClusteredDataGPU::~ezClusteredDataGPU()
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  pDevice->DestroyBuffer(m_hLightDataBuffer);
  pDevice->DestroyBuffer(m_hDecalDataBuffer);
  pDevice->DestroyBuffer(m_hClusterDataBuffer);
  pDevice->DestroyBuffer(m_hClusterItemBuffer);
  pDevice->DestroySamplerState(m_hShadowSampler);
  pDevice->DestroySamplerState(m_hDecalAtlasSampler);

  ezRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void ezClusteredDataGPU::BindResources(ezRenderContext* pRenderContext)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  auto hShadowDataBufferView = pDevice->GetDefaultResourceView(ezShadowPool::GetShadowDataBuffer());
  auto hShadowAtlasTextureView = pDevice->GetDefaultResourceView(ezShadowPool::GetShadowAtlasTexture());

  auto hReflectionSpecularTextureView = pDevice->GetDefaultResourceView(ezReflectionPool::GetReflectionSpecularTexture());
  auto hSkyIrradianceTextureView = pDevice->GetDefaultResourceView(ezReflectionPool::GetSkyIrradianceTexture());

  pRenderContext->BindBuffer("perLightDataBuffer", pDevice->GetDefaultResourceView(m_hLightDataBuffer));
  pRenderContext->BindBuffer("perDecalDataBuffer", pDevice->GetDefaultResourceView(m_hDecalDataBuffer));
  pRenderContext->BindBuffer("perClusterDataBuffer", pDevice->GetDefaultResourceView(m_hClusterDataBuffer));
  pRenderContext->BindBuffer("clusterItemBuffer", pDevice->GetDefaultResourceView(m_hClusterItemBuffer));

  pRenderContext->BindBuffer("shadowDataBuffer", hShadowDataBufferView);
  pRenderContext->BindTexture2D("ShadowAtlasTexture", hShadowAtlasTextureView);
  pRenderContext->BindSamplerState("ShadowSampler", m_hShadowSampler);

  ezResourceLock<ezDecalAtlasResource> pDecalAtlas(m_hDecalAtlas, ezResourceAcquireMode::AllowLoadingFallback);
  pRenderContext->BindTexture2D("DecalAtlasBaseColorTexture", pDecalAtlas->GetBaseColorTexture());
  pRenderContext->BindTexture2D("DecalAtlasNormalTexture", pDecalAtlas->GetNormalTexture());
  pRenderContext->BindTexture2D("DecalAtlasORMTexture", pDecalAtlas->GetORMTexture());
  pRenderContext->BindSamplerState("DecalAtlasSampler", m_hDecalAtlasSampler);

  pRenderContext->BindTextureCube("ReflectionSpecularTexture", hReflectionSpecularTextureView);
  pRenderContext->BindTexture2D("SkyIrradianceTexture", hSkyIrradianceTextureView);

  pRenderContext->BindConstantBuffer("ezClusteredDataConstants", m_hConstantBuffer);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezClusteredDataProvider, 1, ezRTTIDefaultAllocator<ezClusteredDataProvider>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezClusteredDataProvider::ezClusteredDataProvider() {}

ezClusteredDataProvider::~ezClusteredDataProvider() {}

void* ezClusteredDataProvider::UpdateData(const ezRenderViewContext& renderViewContext, const ezExtractedRenderData& extractedData)
{
  ezGALCommandEncoder* pGALCommandEncoder = renderViewContext.m_pRenderContext->GetRenderCommandEncoder();

  EZ_PROFILE_AND_MARKER(pGALCommandEncoder, "Update Clustered Data");

  if (auto pData = extractedData.GetFrameData<ezClusteredDataCPU>())
  {
    // Update buffer
    if (!pData->m_ClusterItemList.IsEmpty())
    {
      if (!pData->m_LightData.IsEmpty())
      {
        pGALCommandEncoder->UpdateBuffer(m_Data.m_hLightDataBuffer, 0, pData->m_LightData.ToByteArray());
      }

      if (!pData->m_DecalData.IsEmpty())
      {
        pGALCommandEncoder->UpdateBuffer(m_Data.m_hDecalDataBuffer, 0, pData->m_DecalData.ToByteArray());
      }

      pGALCommandEncoder->UpdateBuffer(m_Data.m_hClusterItemBuffer, 0, pData->m_ClusterItemList.ToByteArray());
    }

    pGALCommandEncoder->UpdateBuffer(m_Data.m_hClusterDataBuffer, 0, pData->m_ClusterData.ToByteArray());

    // Update Constants
    const ezRectFloat& viewport = renderViewContext.m_pViewData->m_ViewPortRect;

    ezClusteredDataConstants* pConstants =
      renderViewContext.m_pRenderContext->GetConstantBufferData<ezClusteredDataConstants>(m_Data.m_hConstantBuffer);
    pConstants->DepthSliceScale = s_fDepthSliceScale;
    pConstants->DepthSliceBias = s_fDepthSliceBias;
    pConstants->InvTileSize = ezVec2(NUM_CLUSTERS_X / viewport.width, NUM_CLUSTERS_Y / viewport.height);
    pConstants->NumLights = pData->m_LightData.GetCount();
    pConstants->NumDecals = pData->m_DecalData.GetCount();

    pConstants->SkyIrradianceIndex = pData->m_uiSkyIrradianceIndex;

    pConstants->FogHeight = pData->m_fFogHeight;
    pConstants->FogHeightFalloff = pData->m_fFogHeightFalloff;
    pConstants->FogDensityAtCameraPos = pData->m_fFogDensityAtCameraPos;
    pConstants->FogDensity = pData->m_fFogDensity;
    pConstants->FogColor = pData->m_FogColor;
  }

  return &m_Data;
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ClusteredDataProvider);
