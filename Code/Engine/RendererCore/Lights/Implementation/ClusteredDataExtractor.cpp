#include <PCH.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/ClusteredDataExtractor.h>
#include <RendererCore/Lights/Implementation/ClusteredDataUtils.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
ezCVarBool CVarVisClusteredData("r_VisClusteredData", false, ezCVarFlags::Default, "Enables debug visualization of clustered light data");
ezCVarInt CVarVisClusterDepthSlice("r_VisClusterDepthSlice", -1, ezCVarFlags::Default, "Show the debug visualization only for the given depth slice");

namespace
{
  void VisualizeClusteredData(const ezView& view, const ezClusteredDataCPU* pData)
  {
    if (!CVarVisClusteredData)
      return;

    const ezCamera* pCamera = view.GetCullingCamera();
    float fAspectRatio = view.GetViewport().width / view.GetViewport().height;
    float fTanFovX = ezMath::Tan(pCamera->GetFovX(fAspectRatio) * 0.5f);
    float fTanFovY = ezMath::Tan(pCamera->GetFovY(fAspectRatio) * 0.5f);

    ezVec3 pos = pCamera->GetCenterPosition();
    ezVec3 dirForward = pCamera->GetCenterDirForwards();
    ezVec3 dirRight = pCamera->GetCenterDirRight();
    ezVec3 dirUp = pCamera->GetCenterDirUp();

    ezColor lineColor = ezColor(1.0f, 1.0f, 1.0f, 0.1f);

    ezInt32 debugSlice = CVarVisClusterDepthSlice;
    ezUInt32 maxSlice = debugSlice < 0 ? NUM_CLUSTERS_Z : debugSlice + 1;
    ezUInt32 minSlice = debugSlice < 0 ? 0 : debugSlice;

    for (ezUInt32 z = maxSlice; z-- > minSlice;)
    {
      float fDepthFar = GetDepthFromSliceIndex(z);
      float fDepthNear = (z > 0) ? GetDepthFromSliceIndex(z - 1) : 0.0f;

      float fHalfWidthFar = fDepthFar * fTanFovX;
      float fHalfHeightFar = fDepthFar * fTanFovY;
      float fHalfWidthNear = fDepthNear * fTanFovX;
      float fHalfHeightNear = fDepthNear * fTanFovY;

      ezVec3 depthFar = pos + dirForward * fDepthFar;
      ezVec3 depthNear = pos + dirForward * fDepthNear;

      float fStepXf = (fHalfWidthFar * 2.0f) / NUM_CLUSTERS_X;
      float fStepYf = (fHalfHeightFar * 2.0f) / NUM_CLUSTERS_Y;
      float fStepXn = (fHalfWidthNear * 2.0f) / NUM_CLUSTERS_X;
      float fStepYn = (fHalfHeightNear * 2.0f) / NUM_CLUSTERS_Y;

      for (ezInt32 y = 0; y < NUM_CLUSTERS_Y; ++y)
      {
        for (ezInt32 x = 0; x < NUM_CLUSTERS_X; ++x)
        {
          ezUInt32 clusterIndex = GetClusterIndexFromCoord(x, y, z);
          auto& clusterData = pData->m_ClusterData[clusterIndex];

          if (clusterData.counts > 0)
          {
            float fXf = (x - (NUM_CLUSTERS_X / 2)) * fStepXf;
            float fYf = (y - (NUM_CLUSTERS_Y / 2)) * fStepYf;

            ezVec3 tlCornerF = depthFar + dirRight * fXf - dirUp * fYf;
            ezVec3 trCornerF = tlCornerF + dirRight * fStepXf;
            ezVec3 blCornerF = tlCornerF - dirUp * fStepYf;
            ezVec3 brCornerF = blCornerF + dirRight * fStepXf;

            float fXn = (x - (NUM_CLUSTERS_X / 2)) * fStepXn;
            float fYn = (y - (NUM_CLUSTERS_Y / 2)) * fStepYn;

            ezVec3 tlCornerN = depthNear + dirRight * fXn - dirUp * fYn;
            ezVec3 trCornerN = tlCornerN + dirRight * fStepXn;
            ezVec3 blCornerN = tlCornerN - dirUp * fStepYn;
            ezVec3 brCornerN = blCornerN + dirRight * fStepXn;

            float normalizedCount = (clusterData.counts - 1) / 16.0f;
            float r = ezMath::Clamp(normalizedCount, 0.0f, 1.0f);
            float g = ezMath::Clamp(2.0f - normalizedCount, 0.0f, 1.0f);

            ezDebugRenderer::Triangle tris[12];
            //back
            tris[0] = ezDebugRenderer::Triangle(tlCornerF, blCornerF, trCornerF);
            tris[1] = ezDebugRenderer::Triangle(blCornerF, brCornerF, trCornerF);
            //front
            tris[2] = ezDebugRenderer::Triangle(tlCornerN, trCornerN, blCornerN);
            tris[3] = ezDebugRenderer::Triangle(blCornerN, trCornerN, brCornerN);
            //top
            tris[4] = ezDebugRenderer::Triangle(tlCornerN, tlCornerF, trCornerN);
            tris[5] = ezDebugRenderer::Triangle(tlCornerF, trCornerF, trCornerN);
            //bottom
            tris[6] = ezDebugRenderer::Triangle(blCornerN, brCornerN, blCornerF);
            tris[7] = ezDebugRenderer::Triangle(blCornerF, brCornerN, brCornerF);
            //left
            tris[8] = ezDebugRenderer::Triangle(tlCornerN, blCornerN, tlCornerF);
            tris[9] = ezDebugRenderer::Triangle(tlCornerF, blCornerN, blCornerF);
            //right
            tris[10] = ezDebugRenderer::Triangle(trCornerN, trCornerF, brCornerN);
            tris[11] = ezDebugRenderer::Triangle(trCornerF, brCornerF, brCornerN);

            ezDebugRenderer::DrawSolidTriangles(view.GetHandle(), tris, ezColor(r, g, 0.0f, 0.1f));

            ezDebugRenderer::Line lines[12];
            lines[0] = ezDebugRenderer::Line(tlCornerN, trCornerN);
            lines[1] = ezDebugRenderer::Line(trCornerN, brCornerN);
            lines[2] = ezDebugRenderer::Line(brCornerN, blCornerN);
            lines[3] = ezDebugRenderer::Line(blCornerN, tlCornerN);

            lines[4] = ezDebugRenderer::Line(tlCornerF, trCornerF);
            lines[5] = ezDebugRenderer::Line(trCornerF, brCornerF);
            lines[6] = ezDebugRenderer::Line(brCornerF, blCornerF);
            lines[7] = ezDebugRenderer::Line(blCornerF, tlCornerF);

            lines[8] = ezDebugRenderer::Line(tlCornerN, tlCornerF);
            lines[9] = ezDebugRenderer::Line(trCornerN, trCornerF);
            lines[10] = ezDebugRenderer::Line(brCornerN, brCornerF);
            lines[11] = ezDebugRenderer::Line(blCornerN, blCornerF);

            ezDebugRenderer::DrawLines(view.GetHandle(), lines, ezColor(r, g, 0.0f));
          }
        }
      }

      {
        ezVec3 halfWidth = dirRight * fHalfWidthFar;
        ezVec3 halfHeight = dirUp * fHalfHeightFar;

        ezVec3 p0 = depthFar + halfWidth + halfHeight;
        ezVec3 p1 = depthFar + halfWidth - halfHeight;
        ezVec3 p2 = depthFar - halfWidth - halfHeight;
        ezVec3 p3 = depthFar - halfWidth + halfHeight;

        ezDebugRenderer::Line lines[4];
        lines[0] = ezDebugRenderer::Line(p0, p1);
        lines[1] = ezDebugRenderer::Line(p1, p2);
        lines[2] = ezDebugRenderer::Line(p2, p3);
        lines[3] = ezDebugRenderer::Line(p3, p0);

        ezDebugRenderer::DrawLines(view.GetHandle(), lines, lineColor);
      }
    }
  }
}
#endif

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezClusteredDataCPU, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezClusteredDataExtractor, 1, ezRTTIDefaultAllocator<ezClusteredDataExtractor>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezClusteredDataExtractor::ezClusteredDataExtractor(const char* szName)
  : ezExtractor(szName)
{
  m_DependsOn.PushBack(ezMakeHashedString("ezVisibleObjectsExtractor"));

  m_TempClusters.SetCountUninitialized(NUM_CLUSTERS);
}

ezClusteredDataExtractor::~ezClusteredDataExtractor()
{

}

void ezClusteredDataExtractor::PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
  ezExtractedRenderData* pExtractedRenderData)
{
  ezColor ambientTopColor = ezColor::Black;
  ezColor ambientBottomColor = ezColor::Black;

  // Collect and rasterize light data
  m_TempLightData.Clear();
  m_TempClusterItemList.Clear();
  ezMemoryUtils::ZeroFill(m_TempClusters.GetData(), NUM_CLUSTERS);

  const ezCamera* pCamera = view.GetCullingCamera();
  const float fViewportAspectRatio = view.GetViewport().width / view.GetViewport().height;

  ezMat4 viewMatrix;
  pCamera->GetViewMatrix(viewMatrix);

  ezMat4 projectionMatrix;
  pCamera->GetProjectionMatrix(fViewportAspectRatio, projectionMatrix);

  ezSimdMat4f viewProjectionMatrix = ezSimdConversion::ToMat4(projectionMatrix * viewMatrix);
  ezVec3 cameraPosition = pCamera->GetPosition();

  auto batchList = pExtractedRenderData->GetRenderDataBatchesWithCategory(ezDefaultRenderDataCategories::Light);
  const ezUInt32 uiBatchCount = batchList.GetBatchCount();
  for (ezUInt32 i = 0; i < uiBatchCount; ++i)
  {
    const ezRenderDataBatch& batch = batchList.GetBatch(i);

    for (auto it = batch.GetIterator<ezRenderData>(); it.IsValid(); ++it)
    {
      const ezUInt32 uiLightIndex = m_TempLightData.GetCount();

      if (uiLightIndex == ezClusteredDataCPU::MAX_LIGHT_DATA)
      {
        ezLog::Warning("Maximum number of lights reached ({0}). Further lights will be discarded.", ezClusteredDataCPU::MAX_LIGHT_DATA);
        break;
      }

      if (auto pPointLightRenderData = ezDynamicCast<const ezPointLightRenderData*>(it))
      {
        FillPointLightData(m_TempLightData.ExpandAndGetRef(), pPointLightRenderData);

        RasterizePointLight(pPointLightRenderData, uiLightIndex, viewProjectionMatrix, cameraPosition, m_TempClusters.GetArrayPtr());
      }
      else if (auto pSpotLightRenderData = ezDynamicCast<const ezSpotLightRenderData*>(it))
      {
        FillSpotLightData(m_TempLightData.ExpandAndGetRef(), pSpotLightRenderData);

        RasterizeSpotLight(pSpotLightRenderData, uiLightIndex, viewProjectionMatrix, cameraPosition, m_TempClusters.GetArrayPtr());
      }
      else if (auto pDirLightRenderData = ezDynamicCast<const ezDirectionalLightRenderData*>(it))
      {
        FillDirLightData(m_TempLightData.ExpandAndGetRef(), pDirLightRenderData);

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

  ezClusteredDataCPU* pData = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), ezClusteredDataCPU);
  pData->m_LightData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezPerLightData, m_TempLightData.GetCount());
  pData->m_LightData.CopyFrom(m_TempLightData);

  pData->m_AmbientTopColor = ambientTopColor;
  pData->m_AmbientBottomColor = ambientBottomColor;

  FillItemListAndClusterData(pData);

  pExtractedRenderData->AddFrameData(pData);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  VisualizeClusteredData(view, pData);
#endif
}

void ezClusteredDataExtractor::FillItemListAndClusterData(ezClusteredDataCPU* pData)
{
  pData->m_ClusterData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezPerClusterData, NUM_CLUSTERS);

  const ezUInt32 uiNumLights = pData->m_LightData.GetCount();
  const ezUInt32 uiMaxBlockIndex = (uiNumLights + 31) / 32;

  for (ezUInt32 i = 0; i < NUM_CLUSTERS; ++i)
  {
    ezUInt32 uiOffset = m_TempClusterItemList.GetCount();
    ezUInt32 uiCount = 0;

    auto& tempCluster = m_TempClusters[i];
    for (ezUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxBlockIndex; ++uiBlockIndex)
    {
      ezUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

      while (mask > 0)
      {
        ezUInt32 uiLightIndex = ezMath::FirstBitLow(mask);
        mask &= ~(1 << uiLightIndex);

        uiLightIndex += uiBlockIndex * 32;
        m_TempClusterItemList.PushBack(uiLightIndex);
        ++uiCount;
      }
    }

    auto& clusterData = pData->m_ClusterData[i];
    clusterData.offset = uiOffset;
    clusterData.counts = uiCount;
  }

  pData->m_ClusterItemList = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezUInt32, m_TempClusterItemList.GetCount());
  pData->m_ClusterItemList.CopyFrom(m_TempClusterItemList);
}
