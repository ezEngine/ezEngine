#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/ClusteredDataExtractor.h>
#include <RendererCore/Lights/Implementation/ClusteredDataUtils.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
ezCVarBool cvar_RenderingLightingVisClusterData("Rendering.Lighting.VisClusterData", false, ezCVarFlags::Default, "Enables debug visualization of clustered light data");
ezCVarInt cvar_RenderingLightingVisClusterDepthSlice("Rendering.Lighting.VisClusterDepthSlice", -1, ezCVarFlags::Default, "Show the debug visualization only for the given depth slice");

namespace
{
  void VisualizeClusteredData(const ezView& view, const ezClusteredDataCPU* pData, ezArrayPtr<ezSimdBSphere> boundingSpheres)
  {
    if (!cvar_RenderingLightingVisClusterData)
      return;

    const ezCamera* pCamera = view.GetCullingCamera();

    if (pCamera->IsOrthographic())
      return;

    float fAspectRatio = view.GetViewport().width / view.GetViewport().height;

    ezMat4 mProj;
    pCamera->GetProjectionMatrix(fAspectRatio, mProj);

    ezAngle fFovLeft;
    ezAngle fFovRight;
    ezAngle fFovBottom;
    ezAngle fFovTop;
    ezGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fFovLeft, fFovRight, fFovBottom, fFovTop);

    const float fTanLeft = ezMath::Tan(fFovLeft);
    const float fTanRight = ezMath::Tan(fFovRight);
    const float fTanBottom = ezMath::Tan(fFovBottom);
    const float fTanTop = ezMath::Tan(fFovTop);

    ezColor lineColor = ezColor(1.0f, 1.0f, 1.0f, 0.1f);

    ezInt32 debugSlice = cvar_RenderingLightingVisClusterDepthSlice;
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
            GetClusterCornerPoints(*pCamera, fZf, fZn, fTanLeft, fTanRight, fTanBottom, fTanTop, x, y, z, cc);

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
        ezVec3 leftWidth = pCamera->GetDirRight() * fZf * fTanLeft;
        ezVec3 rightWidth = pCamera->GetDirRight() * fZf * fTanRight;
        ezVec3 bottomHeight = pCamera->GetDirUp() * fZf * fTanBottom;
        ezVec3 topHeight = pCamera->GetDirUp() * fZf * fTanTop;

        ezVec3 depthFar = pCamera->GetPosition() + pCamera->GetDirForwards() * fZf;
        ezVec3 p0 = depthFar + rightWidth + topHeight;
        ezVec3 p1 = depthFar + rightWidth + bottomHeight;
        ezVec3 p2 = depthFar + leftWidth + bottomHeight;
        ezVec3 p3 = depthFar + leftWidth + topHeight;

        ezDebugRenderer::Line lines[4];
        lines[0] = ezDebugRenderer::Line(p0, p1);
        lines[1] = ezDebugRenderer::Line(p1, p2);
        lines[2] = ezDebugRenderer::Line(p2, p3);
        lines[3] = ezDebugRenderer::Line(p3, p0);

        ezDebugRenderer::DrawLines(view.GetHandle(), lines, lineColor);
      }
    }
  }
} // namespace
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
  m_TempReflectionProbeClusters.SetCountUninitialized(NUM_CLUSTERS);
  m_ClusterBoundingSpheres.SetCountUninitialized(NUM_CLUSTERS);
}

ezClusteredDataExtractor::~ezClusteredDataExtractor() = default;

void ezClusteredDataExtractor::PostSortAndBatch(const ezView& view, const ezDynamicArray<const ezGameObject*>& visibleObjects, ezExtractedRenderData& ref_extractedRenderData)
{
  EZ_PROFILE_SCOPE("PostSortAndBatch");

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
    EZ_PROFILE_SCOPE("Lights");
    m_TempLightData.Clear();
    ezMemoryUtils::ZeroFill(m_TempLightsClusters.GetData(), NUM_CLUSTERS);

    auto batchList = ref_extractedRenderData.GetRenderDataBatchesWithCategory(ezDefaultRenderDataCategories::Light);
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
          RasterizeSphere(pointLightSphere, uiLightIndex, viewMatrix, projectionMatrix, m_TempLightsClusters.GetData(), m_ClusterBoundingSpheres.GetData());

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
          RasterizeSpotLight(cone, uiLightIndex, viewMatrix, projectionMatrix, m_TempLightsClusters.GetData(), m_ClusterBoundingSpheres.GetData());
        }
        else if (auto pDirLightRenderData = ezDynamicCast<const ezDirectionalLightRenderData*>(it))
        {
          FillDirLightData(m_TempLightData.ExpandAndGetRef(), pDirLightRenderData);

          RasterizeDirLight(pDirLightRenderData, uiLightIndex, m_TempLightsClusters.GetArrayPtr());
        }
        else if (auto pFillLightRenderData = ezDynamicCast<const ezFillLightRenderData*>(it))
        {
          FillFillLightData(m_TempLightData.ExpandAndGetRef(), pFillLightRenderData);

          ezSimdBSphere fillLightSphere = ezSimdBSphere(ezSimdConversion::ToVec3(pFillLightRenderData->m_GlobalTransform.m_vPosition), pFillLightRenderData->m_fRange);
          RasterizeSphere(fillLightSphere, uiLightIndex, viewMatrix, projectionMatrix, m_TempLightsClusters.GetData(), m_ClusterBoundingSpheres.GetData());
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
          pData->m_fFogInvSkyDistance = pFogRenderData->m_fInvSkyDistance;

          pData->m_FogColor = pFogRenderData->m_Color;
        }
        else
        {
          ezLog::Warning("Unhandled render data type '{}' in 'Light' category", it->GetDynamicRTTI()->GetTypeName());
        }
      }
    }

    pData->m_LightData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezPerLightData, m_TempLightData.GetCount());
    pData->m_LightData.CopyFrom(m_TempLightData);

    pData->m_uiSkyIrradianceIndex = view.GetWorld()->GetIndex();
    pData->m_cameraUsageHint = view.GetCameraUsageHint();
  }

  // Decals
  {
    EZ_PROFILE_SCOPE("Decals");
    m_TempDecalData.Clear();
    ezMemoryUtils::ZeroFill(m_TempDecalsClusters.GetData(), NUM_CLUSTERS);

    auto batchList = ref_extractedRenderData.GetRenderDataBatchesWithCategory(ezDefaultRenderDataCategories::Decal);
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

          RasterizeBox(pDecalRenderData->m_GlobalTransform, uiDecalIndex, viewProjectionMatrix, m_TempDecalsClusters.GetData(), m_ClusterBoundingSpheres.GetData());
        }
        else
        {
          ezLog::Warning("Unhandled render data type '{}' in 'Decal' category", it->GetDynamicRTTI()->GetTypeName());
        }
      }
    }

    pData->m_DecalData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezPerDecalData, m_TempDecalData.GetCount());
    pData->m_DecalData.CopyFrom(m_TempDecalData);
  }

  // Reflection Probes
  {
    EZ_PROFILE_SCOPE("Probes");
    m_TempReflectionProbeData.Clear();
    ezMemoryUtils::ZeroFill(m_TempReflectionProbeClusters.GetData(), NUM_CLUSTERS);

    auto batchList = ref_extractedRenderData.GetRenderDataBatchesWithCategory(ezDefaultRenderDataCategories::ReflectionProbe);
    const ezUInt32 uiBatchCount = batchList.GetBatchCount();
    for (ezUInt32 i = 0; i < uiBatchCount; ++i)
    {
      const ezRenderDataBatch& batch = batchList.GetBatch(i);

      for (auto it = batch.GetIterator<ezRenderData>(); it.IsValid(); ++it)
      {
        const ezUInt32 uiProbeIndex = m_TempReflectionProbeData.GetCount();

        if (uiProbeIndex == ezClusteredDataCPU::MAX_REFLECTION_PROBE_DATA)
        {
          ezLog::Warning("Maximum number of reflection probes reached ({0}). Further reflection probes will be discarded.", ezClusteredDataCPU::MAX_REFLECTION_PROBE_DATA);
          break;
        }

        if (auto pReflectionProbeRenderData = ezDynamicCast<const ezReflectionProbeRenderData*>(it))
        {
          auto& probeData = m_TempReflectionProbeData.ExpandAndGetRef();
          FillReflectionProbeData(probeData, pReflectionProbeRenderData);

          const ezVec3 vFullScale = pReflectionProbeRenderData->m_vHalfExtents.CompMul(pReflectionProbeRenderData->m_GlobalTransform.m_vScale);

          bool bRasterizeSphere = false;
          float fMaxRadius = 0.0f;
          if (pReflectionProbeRenderData->m_uiIndex & REFLECTION_PROBE_IS_SPHERE)
          {
            constexpr float fSphereConstant = (4.0f / 3.0f) * ezMath::Pi<float>();
            fMaxRadius = ezMath::Max(ezMath::Max(ezMath::Abs(vFullScale.x), ezMath::Abs(vFullScale.y)), ezMath::Abs(vFullScale.z));
            const float fSphereVolume = fSphereConstant * ezMath::Pow(fMaxRadius, 3.0f);
            const float fBoxVolume = ezMath::Abs(vFullScale.x * vFullScale.y * vFullScale.z * 8);
            if (fSphereVolume < fBoxVolume)
            {
              bRasterizeSphere = true;
            }
          }

          if (bRasterizeSphere)
          {
            ezSimdBSphere pointLightSphere =
              ezSimdBSphere(ezSimdConversion::ToVec3(pReflectionProbeRenderData->m_GlobalTransform.m_vPosition), fMaxRadius);
            RasterizeSphere(
              pointLightSphere, uiProbeIndex, viewMatrix, projectionMatrix, m_TempReflectionProbeClusters.GetData(), m_ClusterBoundingSpheres.GetData());
          }
          else
          {
            ezTransform transform = pReflectionProbeRenderData->m_GlobalTransform;
            transform.m_vScale = vFullScale.CompMul(probeData.InfluenceScale.GetAsVec3());
            transform.m_vPosition += transform.m_qRotation * vFullScale.CompMul(probeData.InfluenceShift.GetAsVec3());

            // const ezBoundingBox aabb(ezVec3(-1.0f), ezVec3(1.0f));
            // ezDebugRenderer::DrawLineBox(view.GetHandle(), aabb, ezColor::DarkBlue, transform);

            RasterizeBox(transform, uiProbeIndex, viewProjectionMatrix, m_TempReflectionProbeClusters.GetData(), m_ClusterBoundingSpheres.GetData());
          }
        }
        else
        {
          ezLog::Warning("Unhandled render data type '{}' in 'ReflectionProbe' category", it->GetDynamicRTTI()->GetTypeName());
        }
      }
    }

    pData->m_ReflectionProbeData = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezPerReflectionProbeData, m_TempReflectionProbeData.GetCount());
    pData->m_ReflectionProbeData.CopyFrom(m_TempReflectionProbeData);
  }

  FillItemListAndClusterData(pData);

  ref_extractedRenderData.AddFrameData(pData);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  VisualizeClusteredData(view, pData, m_ClusterBoundingSpheres);
#endif
}

ezResult ezClusteredDataExtractor::Serialize(ezStreamWriter& inout_stream) const
{
  EZ_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return EZ_SUCCESS;
}


ezResult ezClusteredDataExtractor::Deserialize(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const ezUInt32 uiVersion = ezTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  EZ_IGNORE_UNUSED(uiVersion);
  return EZ_SUCCESS;
}

namespace
{
  ezUInt32 PackIndex(ezUInt32 uiLightIndex, ezUInt32 uiDecalIndex)
  {
    return uiDecalIndex << 10 | uiLightIndex;
  }

  ezUInt32 PackReflectionProbeIndex(ezUInt32 uiData, ezUInt32 uiReflectionProbeIndex)
  {
    return uiReflectionProbeIndex << 20 | uiData;
  }
} // namespace

void ezClusteredDataExtractor::FillItemListAndClusterData(ezClusteredDataCPU* pData)
{
  EZ_PROFILE_SCOPE("FillItemListAndClusterData");
  m_TempClusterItemList.Clear();

  const ezUInt32 uiNumLights = m_TempLightData.GetCount();
  const ezUInt32 uiMaxLightBlockIndex = (uiNumLights + 31) / 32;

  const ezUInt32 uiNumDecals = m_TempDecalData.GetCount();
  const ezUInt32 uiMaxDecalBlockIndex = (uiNumDecals + 31) / 32;

  const ezUInt32 uiNumReflectionProbes = m_TempReflectionProbeData.GetCount();
  const ezUInt32 uiMaxReflectionProbeBlockIndex = (uiNumReflectionProbes + 31) / 32;

  const ezUInt32 uiWorstCase = ezMath::Max(uiNumLights, uiNumDecals, uiNumReflectionProbes);
  for (ezUInt32 i = 0; i < NUM_CLUSTERS; ++i)
  {
    const ezUInt32 uiOffset = m_TempClusterItemList.GetCount();
    ezUInt32 uiLightCount = 0;

    // We expand m_TempClusterItemList by the worst case this loop can produce and then cut it down again to the actual size once we have filled the data. This makes sure we do not waste time on boundary checks or potential out of line calls like PushBack or PushBackUnchecked.
    m_TempClusterItemList.SetCountUninitialized(uiOffset + uiWorstCase);
    ezUInt32* pTempClusterItemListRange = m_TempClusterItemList.GetData() + uiOffset;

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
          pTempClusterItemListRange[uiLightCount] = uiLightIndex;
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
            auto& item = pTempClusterItemListRange[uiDecalCount];
            item = PackIndex(item, uiDecalIndex);
          }
          else
          {
            pTempClusterItemListRange[uiDecalCount] = PackIndex(0, uiDecalIndex);
          }

          ++uiDecalCount;
        }
      }
    }

    ezUInt32 uiReflectionProbeCount = 0;
    const ezUInt32 uiMaxUsed = ezMath::Max(uiLightCount, uiDecalCount);
    // Reflection Probes
    {
      auto& tempCluster = m_TempReflectionProbeClusters[i];
      for (ezUInt32 uiBlockIndex = 0; uiBlockIndex < uiMaxReflectionProbeBlockIndex; ++uiBlockIndex)
      {
        ezUInt32 mask = tempCluster.m_BitMask[uiBlockIndex];

        while (mask > 0)
        {
          ezUInt32 uiReflectionProbeIndex = ezMath::FirstBitLow(mask);
          mask &= ~(1 << uiReflectionProbeIndex);

          uiReflectionProbeIndex += uiBlockIndex * 32;

          if (uiReflectionProbeCount < uiMaxUsed)
          {
            auto& item = pTempClusterItemListRange[uiReflectionProbeCount];
            item = PackReflectionProbeIndex(item, uiReflectionProbeIndex);
          }
          else
          {
            pTempClusterItemListRange[uiReflectionProbeCount] = PackReflectionProbeIndex(0, uiReflectionProbeIndex);
          }

          ++uiReflectionProbeCount;
        }
      }
    }

    // Cut down the array to the actual number of elements we have written.
    const ezUInt32 uiActualCase = ezMath::Max(uiLightCount, uiDecalCount, uiReflectionProbeCount);
    m_TempClusterItemList.SetCountUninitialized(uiOffset + uiActualCase);

    auto& clusterData = pData->m_ClusterData[i];
    clusterData.offset = uiOffset;
    clusterData.counts = PackReflectionProbeIndex(PackIndex(uiLightCount, uiDecalCount), uiReflectionProbeCount);
  }

  pData->m_ClusterItemList = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), ezUInt32, m_TempClusterItemList.GetCount());
  pData->m_ClusterItemList.CopyFrom(m_TempClusterItemList);
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ClusteredDataExtractor);
