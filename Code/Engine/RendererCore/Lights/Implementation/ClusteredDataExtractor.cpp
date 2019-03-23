#include <RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/ClusteredDataExtractor.h>
#include <RendererCore/Lights/Implementation/ClusteredDataUtils.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
ezCVarBool CVarVisClusteredData("r_VisClusteredData", false, ezCVarFlags::Default, "Enables debug visualization of clustered light data");
ezCVarInt CVarVisClusterDepthSlice("r_VisClusterDepthSlice", -1, ezCVarFlags::Default,
                                   "Show the debug visualization only for the given depth slice");

namespace
{
  void VisualizeClusteredData(const ezView& view, const ezClusteredDataCPU* pData, ezArrayPtr<ezSimdBSphere> boundingSpheres)
  {
    if (!CVarVisClusteredData)
      return;

    const ezCamera* pCamera = view.GetCullingCamera();

    if (pCamera->IsOrthographic())
      return;

    float fAspectRatio = view.GetViewport().width / view.GetViewport().height;
    float fTanFovX = ezMath::Tan(pCamera->GetFovX(fAspectRatio) * 0.5f) * 2.0f;
    float fTanFovY = ezMath::Tan(pCamera->GetFovY(fAspectRatio) * 0.5f) * 2.0f;

    ezColor lineColor = ezColor(1.0f, 1.0f, 1.0f, 0.1f);

    ezInt32 debugSlice = CVarVisClusterDepthSlice;
    ezUInt32 maxSlice = debugSlice < 0 ? NUM_CLUSTERS_Z : debugSlice + 1;
    ezUInt32 minSlice = debugSlice < 0 ? 0 : debugSlice;

    bool bDrawBoundingSphere = false;

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

          if (clusterData.counts > 0)
          {
            if (bDrawBoundingSphere)
            {
              ezBoundingSphere s = ezSimdConversion::ToBSphere(boundingSpheres[clusterIndex]);
              ezDebugRenderer::DrawLineSphere(view.GetHandle(), s, lineColor);
            }

            ezVec3 cc[8];
            GetClusterCornerPoints(*pCamera, fZf, fZn, fTanFovX, fTanFovY, x, y, z, cc);

            float lightCount = (float)GET_LIGHT_INDEX(clusterData.counts);
            float decalCount = (float)GET_DECAL_INDEX(clusterData.counts);
            float r = ezMath::Clamp(lightCount / 16.0f, 0.0f, 1.0f);
            float g = ezMath::Clamp(decalCount / 16.0f, 0.0f, 1.0f);

            ezDebugRenderer::Triangle tris[12];
            // back
            tris[0] = ezDebugRenderer::Triangle(cc[0], cc[2], cc[1]);
            tris[1] = ezDebugRenderer::Triangle(cc[2], cc[3], cc[1]);
            // front
            tris[2] = ezDebugRenderer::Triangle(cc[4], cc[5], cc[6]);
            tris[3] = ezDebugRenderer::Triangle(cc[6], cc[5], cc[7]);
            // top
            tris[4] = ezDebugRenderer::Triangle(cc[4], cc[0], cc[5]);
            tris[5] = ezDebugRenderer::Triangle(cc[0], cc[1], cc[5]);
            // bottom
            tris[6] = ezDebugRenderer::Triangle(cc[6], cc[7], cc[2]);
            tris[7] = ezDebugRenderer::Triangle(cc[2], cc[7], cc[3]);
            // left
            tris[8] = ezDebugRenderer::Triangle(cc[4], cc[6], cc[0]);
            tris[9] = ezDebugRenderer::Triangle(cc[0], cc[6], cc[2]);
            // right
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
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezClusteredDataCPU::ezClusteredDataCPU() = default;
ezClusteredDataCPU::~ezClusteredDataCPU() = default;

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezClusteredDataExtractor, 1, ezRTTIDefaultAllocator<ezClusteredDataExtractor>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezClusteredDataExtractor::ezClusteredDataExtractor(const char* szName)
    : ezExtractor(szName)
{
  m_DependsOn.PushBack(ezMakeHashedString("ezVisibleObjectsExtractor"));

  m_TempLightsClusters.SetCountUninitialized(NUM_CLUSTERS);
  m_TempDecalsClusters.SetCountUninitialized(NUM_CLUSTERS);
  m_ClusterBoundingSpheres.SetCountUninitialized(NUM_CLUSTERS);
}

ezClusteredDataExtractor::~ezClusteredDataExtractor() {}

void ezClusteredDataExtractor::PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects,
                                                ezExtractedRenderData& extractedRenderData)
{
  const ezCamera* pCamera = view.GetCullingCamera();
  const float fAspectRatio = view.GetViewport().width / view.GetViewport().height;

  FillClusterBoundingSpheres(*pCamera, fAspectRatio, m_ClusterBoundingSpheres);
  ezClusteredDataCPU* pData = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), ezClusteredDataCPU);
  pData->m_ClusterData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezPerClusterData, NUM_CLUSTERS);

  ezMat4 tmp = pCamera->GetViewMatrix();
  ezSimdMat4f viewMatrix = ezSimdConversion::ToMat4(tmp);

  pCamera->GetProjectionMatrix(fAspectRatio, tmp);
  ezSimdMat4f projectionMatrix = ezSimdConversion::ToMat4(tmp);

  ezSimdMat4f viewProjectionMatrix = projectionMatrix * viewMatrix;

  // Lights
  {
    m_TempLightData.Clear();
    ezMemoryUtils::ZeroFill(m_TempLightsClusters.GetData(), NUM_CLUSTERS);

    auto batchList = extractedRenderData.GetRenderDataBatchesWithCategory(ezDefaultRenderDataCategories::Light);
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

          ezSimdBSphere pointLightSphere = ezSimdBSphere(ezSimdConversion::ToVec3(pPointLightRenderData->m_GlobalTransform.m_vPosition),
                                                         pPointLightRenderData->m_fRange);
          RasterizePointLight(pointLightSphere, uiLightIndex, viewMatrix, projectionMatrix, m_TempLightsClusters.GetData(),
                              m_ClusterBoundingSpheres.GetData());

          if (false)
          {
            ezSimdBBox ssb = GetScreenSpaceBounds(pointLightSphere, viewMatrix, projectionMatrix);
            float minX = ((float)ssb.m_Min.x() * 0.5f + 0.5f) * view.GetViewport().width;
            float maxX = ((float)ssb.m_Max.x() * 0.5f + 0.5f) * view.GetViewport().width;
            float minY = ((float)ssb.m_Max.y() * -0.5f + 0.5f) * view.GetViewport().height;
            float maxY = ((float)ssb.m_Min.y() * -0.5f + 0.5f) * view.GetViewport().height;

            ezRectFloat rect(minX, minY, maxX - minX, maxY - minY);
            ezDebugRenderer::Draw2DRectangle(view.GetHandle(), rect, 0.0f, ezColor::Blue.WithAlpha(0.3f));
          }
        }
        else if (auto pSpotLightRenderData = ezDynamicCast<const ezSpotLightRenderData*>(it))
        {
          FillSpotLightData(m_TempLightData.ExpandAndGetRef(), pSpotLightRenderData);

          ezAngle halfAngle = pSpotLightRenderData->m_OuterSpotAngle / 2.0f;

          BoundingCone cone;
          cone.m_PositionAndRange = ezSimdConversion::ToVec3(pSpotLightRenderData->m_GlobalTransform.m_vPosition);
          cone.m_PositionAndRange.SetW(pSpotLightRenderData->m_fRange);
          cone.m_ForwardDir = ezSimdConversion::ToVec3(pSpotLightRenderData->m_GlobalTransform.m_qRotation * ezVec3(1.0f, 0.0f, 0.0f));
          cone.m_SinCosAngle = ezSimdVec4f(ezMath::Sin(halfAngle), ezMath::Cos(halfAngle), 0.0f);
          RasterizeSpotLight(cone, uiLightIndex, viewMatrix, projectionMatrix, m_TempLightsClusters.GetData(),
                             m_ClusterBoundingSpheres.GetData());
        }
        else if (auto pDirLightRenderData = ezDynamicCast<const ezDirectionalLightRenderData*>(it))
        {
          FillDirLightData(m_TempLightData.ExpandAndGetRef(), pDirLightRenderData);

          RasterizeDirLight(pDirLightRenderData, uiLightIndex, m_TempLightsClusters.GetArrayPtr());
        }
        else if (auto pFogRenderData = ezDynamicCast<const ezFogRenderData*>(it))
        {
          float fogBaseHeight = pFogRenderData->m_GlobalTransform.m_vPosition.z;
          float fogHeightFalloff = pFogRenderData->m_fHeightFalloff > 0.0f ? ezMath::Ln(0.0001f) / pFogRenderData->m_fHeightFalloff : 0.0f;

          float fogAtCameraPos = fogHeightFalloff * (pCamera->GetPosition().z - fogBaseHeight);
          if (fogAtCameraPos >= 80.0f) // Prevent infs
          {
            fogHeightFalloff = 0.0f;
          }

          pData->m_fFogHeight = -fogHeightFalloff * fogBaseHeight;
          pData->m_fFogHeightFalloff = fogHeightFalloff;
          pData->m_fFogDensityAtCameraPos = ezMath::Exp(ezMath::Clamp(fogAtCameraPos, -80.0f, 80.0f)); // Prevent infs
          pData->m_fFogDensity = pFogRenderData->m_fDensity;

          pData->m_FogColor = pFogRenderData->m_Color;
        }
        else
        {
          EZ_ASSERT_NOT_IMPLEMENTED;
        }
      }
    }

    pData->m_LightData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezPerLightData, m_TempLightData.GetCount());
    pData->m_LightData.CopyFrom(m_TempLightData);
  }

  // Decals
  {
    m_TempDecalData.Clear();
    ezMemoryUtils::ZeroFill(m_TempDecalsClusters.GetData(), NUM_CLUSTERS);

    auto batchList = extractedRenderData.GetRenderDataBatchesWithCategory(ezDefaultRenderDataCategories::Decal);
    const ezUInt32 uiBatchCount = batchList.GetBatchCount();
    for (ezUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const ezRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<ezRenderData>(); it.IsValid(); ++it)
      {
        const ezUInt32 uiDecalIndex = m_TempDecalData.GetCount();

        if (uiDecalIndex == ezClusteredDataCPU::MAX_DECAL_DATA)
        {
          ezLog::Warning("Maximum number of decals reached ({0}). Further decals will be discarded.", ezClusteredDataCPU::MAX_DECAL_DATA);
          break;
        }

        if (auto pDecalRenderData = ezDynamicCast<const ezDecalRenderData*>(it))
        {
          FillDecalData(m_TempDecalData.ExpandAndGetRef(), pDecalRenderData);

          RasterizeDecal(pDecalRenderData, uiDecalIndex, viewProjectionMatrix, m_TempDecalsClusters.GetData(),
                         m_ClusterBoundingSpheres.GetData());
        }
        else
        {
          EZ_ASSERT_NOT_IMPLEMENTED;
        }
      }
    }

    pData->m_DecalData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezPerDecalData, m_TempDecalData.GetCount());
    pData->m_DecalData.CopyFrom(m_TempDecalData);
  }

  FillItemListAndClusterData(pData);

  extractedRenderData.AddFrameData(pData);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  VisualizeClusteredData(view, pData, m_ClusterBoundingSpheres);
#endif
}

namespace
{
  ezUInt32 PackIndex(ezUInt32 uiLightIndex, ezUInt32 uiDecalIndex) { return uiDecalIndex << 10 | uiLightIndex; }
}

void ezClusteredDataExtractor::FillItemListAndClusterData(ezClusteredDataCPU* pData)
{
  m_TempClusterItemList.Clear();

  const ezUInt32 uiNumLights = m_TempLightData.GetCount();
  const ezUInt32 uiMaxLightBlockIndex = (uiNumLights + 31) / 32;

  const ezUInt32 uiNumDecals = m_TempDecalData.GetCount();
  const ezUInt32 uiMaxDecalBlockIndex = (uiNumDecals + 31) / 32;

  for (ezUInt32 i = 0; i < NUM_CLUSTERS; ++i)
  {
    ezUInt32 uiOffset = m_TempClusterItemList.GetCount();
    ezUInt32 uiLightCount = 0;

    // Lights
    {
      auto& tempCluster = m_TempLightsClusters[i];
      for (ezUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxLightBlockIndex; ++uiBlockIndex)
      {
        ezUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

        while (mask > 0)
        {
          ezUInt32 uiLightIndex = ezMath::FirstBitLow(mask);
          mask &= ~(1 << uiLightIndex);

          uiLightIndex += uiBlockIndex * 32;
          m_TempClusterItemList.PushBack(uiLightIndex);
          ++uiLightCount;
        }
      }
    }

    ezUInt32 uiDecalCount = 0;

    // Decals
    {
      auto& tempCluster = m_TempDecalsClusters[i];
      for (ezUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxDecalBlockIndex; ++uiBlockIndex)
      {
        ezUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

        while (mask > 0)
        {
          ezUInt32 uiDecalIndex = ezMath::FirstBitLow(mask);
          mask &= ~(1 << uiDecalIndex);

          uiDecalIndex += uiBlockIndex * 32;

          if (uiDecalCount < uiLightCount)
          {
            auto& item = m_TempClusterItemList[uiOffset + uiDecalCount];
            item = PackIndex(item, uiDecalIndex);
          }
          else
          {
            auto& item = m_TempClusterItemList.ExpandAndGetRef();
            item = PackIndex(0, uiDecalIndex);
          }

          ++uiDecalCount;
        }
      }
    }

    auto& clusterData = pData->m_ClusterData[i];
    clusterData.offset = uiOffset;
    clusterData.counts = PackIndex(uiLightCount, uiDecalCount);
  }

  pData->m_ClusterItemList = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezUInt32, m_TempClusterItemList.GetCount());
  pData->m_ClusterItemList.CopyFrom(m_TempClusterItemList);
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ClusteredDataExtractor);

