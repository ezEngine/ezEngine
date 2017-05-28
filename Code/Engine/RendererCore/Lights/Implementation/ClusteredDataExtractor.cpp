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
  void VisualizeClusteredData(const ezView& view, const ezClusteredDataCPU* pData, ezArrayPtr<ezSimdBSphere> boundingSpheres)
  {
    if (!CVarVisClusteredData)
      return;

    const ezCamera* pCamera = view.GetCullingCamera();
    float fAspectRatio = view.GetViewport().width / view.GetViewport().height;
    float fTanFovX = ezMath::Tan(pCamera->GetFovX(fAspectRatio) * 0.5f) * 2.0f;
    float fTanFovY = ezMath::Tan(pCamera->GetFovY(fAspectRatio) * 0.5f) * 2.0f;

    ezColor lineColor = ezColor(1.0f, 1.0f, 1.0f, 0.1f);

    ezInt32 debugSlice = CVarVisClusterDepthSlice;
    ezUInt32 maxSlice = debugSlice < 0 ? NUM_CLUSTERS_Z : debugSlice + 1;
    ezUInt32 minSlice = debugSlice < 0 ? 0 : debugSlice;

    for (ezUInt32 z = maxSlice; z-- > minSlice;)
    {
      float fZf = GetDepthFromSliceIndex(z);
      float fZn = (z > 0) ? GetDepthFromSliceIndex(z - 1) : 0.0f;
      for (ezInt32 y = 0; y < NUM_CLUSTERS_Y; ++y)
      {
        for (ezInt32 x = 0; x < NUM_CLUSTERS_X; ++x)
        {
          ezUInt32 clusterIndex = GetClusterIndexFromCoord(x, y, z);
          auto& clusterData = pData->m_ClusterData[clusterIndex];

          if (false)
          {
            ezBoundingSphere s = ezSimdConversion::ToBSphere(boundingSpheres[clusterIndex]);
            ezDebugRenderer::DrawLineSphere(view.GetHandle(), s, lineColor);
          }

          if (clusterData.counts > 0)
          {
            ezVec3 cc[8];
            GetClusterCornerPoints(*pCamera, fZf, fZn, fTanFovX, fTanFovY, x, y, z, cc);

            float normalizedCount = (clusterData.counts - 1) / 16.0f;
            float r = ezMath::Clamp(normalizedCount, 0.0f, 1.0f);
            float g = ezMath::Clamp(2.0f - normalizedCount, 0.0f, 1.0f);

            ezDebugRenderer::Triangle tris[12];
            //back
            tris[0] = ezDebugRenderer::Triangle(cc[0], cc[2], cc[1]);
            tris[1] = ezDebugRenderer::Triangle(cc[2], cc[3], cc[1]);
            //front
            tris[2] = ezDebugRenderer::Triangle(cc[4], cc[5], cc[6]);
            tris[3] = ezDebugRenderer::Triangle(cc[6], cc[5], cc[7]);
            //top
            tris[4] = ezDebugRenderer::Triangle(cc[4], cc[0], cc[5]);
            tris[5] = ezDebugRenderer::Triangle(cc[0], cc[1], cc[5]);
            //bottom
            tris[6] = ezDebugRenderer::Triangle(cc[6], cc[7], cc[2]);
            tris[7] = ezDebugRenderer::Triangle(cc[2], cc[7], cc[3]);
            //left
            tris[8] = ezDebugRenderer::Triangle(cc[4], cc[6], cc[0]);
            tris[9] = ezDebugRenderer::Triangle(cc[0], cc[6], cc[2]);
            //right
            tris[10] = ezDebugRenderer::Triangle(cc[5], cc[1], cc[7]);
            tris[11] = ezDebugRenderer::Triangle(cc[1], cc[3], cc[7]);

            ezDebugRenderer::DrawSolidTriangles(view.GetHandle(), tris, ezColor(r, g, 0.0f, 0.1f));

            ezDebugRenderer::Line lines[12];
            lines[0] = ezDebugRenderer::Line(cc[4], cc[5]);
            lines[1] = ezDebugRenderer::Line(cc[5], cc[7]);
            lines[2] = ezDebugRenderer::Line(cc[7], cc[6]);
            lines[3] = ezDebugRenderer::Line(cc[6], cc[4]);

            lines[4] = ezDebugRenderer::Line(cc[0], cc[1]);
            lines[5] = ezDebugRenderer::Line(cc[1], cc[3]);
            lines[6] = ezDebugRenderer::Line(cc[3], cc[2]);
            lines[7] = ezDebugRenderer::Line(cc[2], cc[0]);

            lines[8] = ezDebugRenderer::Line(cc[4], cc[0]);
            lines[9] = ezDebugRenderer::Line(cc[5], cc[1]);
            lines[10] = ezDebugRenderer::Line(cc[7], cc[3]);
            lines[11] = ezDebugRenderer::Line(cc[6], cc[2]);

            ezDebugRenderer::DrawLines(view.GetHandle(), lines, ezColor(r, g, 0.0f));
          }
        }
      }

      {
        ezVec3 halfWidth = pCamera->GetDirRight() * fZf * fTanFovX * 0.5f;
        ezVec3 halfHeight = pCamera->GetDirUp() * fZf * fTanFovY * 0.5f;

        ezVec3 depthFar = pCamera->GetPosition() + pCamera->GetDirForwards() * fZf;
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
  m_ClusterBoundingSpheres.SetCountUninitialized(NUM_CLUSTERS);
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
  const float fAspectRatio = view.GetViewport().width / view.GetViewport().height;

  FillClusterBoundingSpheres(*pCamera, fAspectRatio, m_ClusterBoundingSpheres);

  ezMat4 viewMatrix;
  pCamera->GetViewMatrix(viewMatrix);

  ezMat4 projectionMatrix;
  pCamera->GetProjectionMatrix(fAspectRatio, projectionMatrix);

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

        ezSimdBSphere pointLightSphere = ezSimdBSphere(ezSimdConversion::ToVec3(pPointLightRenderData->m_GlobalTransform.m_vPosition), pPointLightRenderData->m_fRange);
        RasterizePointLight(pointLightSphere, uiLightIndex, *pCamera, m_TempClusters.GetData(), m_ClusterBoundingSpheres.GetData());
      }
      else if (auto pSpotLightRenderData = ezDynamicCast<const ezSpotLightRenderData*>(it))
      {
        FillSpotLightData(m_TempLightData.ExpandAndGetRef(), pSpotLightRenderData);

        ezAngle halfAngle = pSpotLightRenderData->m_OuterSpotAngle / 2.0f;

        BoundingCone cone;
        cone.m_PositionAndRange = ezSimdConversion::ToVec3(pSpotLightRenderData->m_GlobalTransform.m_vPosition);
        cone.m_PositionAndRange.SetW(pSpotLightRenderData->m_fRange);
        cone.m_ForwardDir = ezSimdConversion::ToVec3(pSpotLightRenderData->m_GlobalTransform.m_Rotation.TransformDirection(ezVec3(1.0f, 0.0f, 0.0f)));
        cone.m_SinCosAngle = ezSimdVec4f(ezMath::Sin(halfAngle), ezMath::Cos(halfAngle), 0.0f);
        RasterizeSpotLight(cone, uiLightIndex, *pCamera, m_TempClusters.GetData(), m_ClusterBoundingSpheres.GetData());
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
  VisualizeClusteredData(view, pData, m_ClusterBoundingSpheres);
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
