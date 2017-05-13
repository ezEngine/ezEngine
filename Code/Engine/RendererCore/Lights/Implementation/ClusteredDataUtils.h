#pragma once

#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>
EZ_DEFINE_AS_POD_TYPE(ezPerClusterData);

#include <Foundation/Math/Float16.h>
#include <Foundation/SimdMath/SimdConversion.h>

namespace
{
  ///\todo Make this configurable.
  static float s_fMinLightDistance = 5.0f;
  static float s_fMaxLightDistance = 500.0f;

  static float s_fDepthSliceScale = (NUM_CLUSTERS_Z - 1) / (ezMath::Log2(s_fMaxLightDistance) - ezMath::Log2(s_fMinLightDistance));
  static float s_fDepthSliceBias = -s_fDepthSliceScale * ezMath::Log2(s_fMinLightDistance) + 1.0f;

  EZ_ALWAYS_INLINE float GetDepthFromSliceIndex(ezUInt32 uiSliceIndex)
  {
    return ezMath::Pow(2.0f, (uiSliceIndex - s_fDepthSliceBias + 1.0f) / s_fDepthSliceScale);
  }

  EZ_ALWAYS_INLINE ezUInt32 GetSliceIndexFromDepth(float fLinearDepth)
  {
    return ezMath::Clamp((ezInt32)(ezMath::Log2(fLinearDepth) * s_fDepthSliceScale + s_fDepthSliceBias), 0, NUM_CLUSTERS_Z - 1);
  }

  EZ_ALWAYS_INLINE ezUInt32 GetClusterIndexFromCoord(ezUInt32 x, ezUInt32 y, ezUInt32 z)
  {
    return z * NUM_CLUSTERS_XY + y * NUM_CLUSTERS_X + x;
  }

  EZ_ALWAYS_INLINE ezUInt32 Float3ToRGB10(ezVec3 value)
  {
    ezVec3 unsignedValue = value * 0.5f + ezVec3(0.5f);

    ezUInt32 r = ezMath::Clamp(static_cast<ezUInt32>(unsignedValue.x * 1023.0f + 0.5f), 0u, 1023u);
    ezUInt32 g = ezMath::Clamp(static_cast<ezUInt32>(unsignedValue.y * 1023.0f + 0.5f), 0u, 1023u);
    ezUInt32 b = ezMath::Clamp(static_cast<ezUInt32>(unsignedValue.z * 1023.0f + 0.5f), 0u, 1023u);

    return r | (g << 10) | (b << 20);
  }

  EZ_ALWAYS_INLINE ezUInt32 Float2ToRG16F(ezVec2 value)
  {
    ezUInt32 r = ezFloat16(value.x).GetRawData();
    ezUInt32 g = ezFloat16(value.y).GetRawData();

    return r | (g << 16);
  }

  EZ_ALWAYS_INLINE void FillLightData(ezPerLightData& perLightData, const ezLightRenderData* pLightRenderData, ezUInt32 uiType)
  {
    ezMemoryUtils::ZeroFill(&perLightData);

    ezColorLinearUB lightColor = pLightRenderData->m_LightColor;
    lightColor.a = uiType;

    perLightData.colorAndType = *reinterpret_cast<ezUInt32*>(&lightColor.r);
    perLightData.intensity = pLightRenderData->m_fIntensity;
    perLightData.shadowDataOffset = pLightRenderData->m_uiShadowDataOffset;
  }

  void FillPointLightData(ezPerLightData& perLightData, const ezPointLightRenderData* pPointLightRenderData)
  {
    FillLightData(perLightData, pPointLightRenderData, LIGHT_TYPE_POINT);

    perLightData.position = pPointLightRenderData->m_GlobalTransform.m_vPosition;
    perLightData.invSqrAttRadius = 1.0f / (pPointLightRenderData->m_fRange * pPointLightRenderData->m_fRange);
  }

  void FillSpotLightData(ezPerLightData& perLightData, const ezSpotLightRenderData* pSpotLightRenderData)
  {
    FillLightData(perLightData, pSpotLightRenderData, LIGHT_TYPE_SPOT);

    perLightData.direction = Float3ToRGB10(-pSpotLightRenderData->m_GlobalTransform.m_Rotation.GetColumn(0));
    perLightData.position = pSpotLightRenderData->m_GlobalTransform.m_vPosition;
    perLightData.invSqrAttRadius = 1.0f / (pSpotLightRenderData->m_fRange * pSpotLightRenderData->m_fRange);

    const float fCosInner = ezMath::Cos(pSpotLightRenderData->m_InnerSpotAngle * 0.5f);
    const float fCosOuter = ezMath::Cos(pSpotLightRenderData->m_OuterSpotAngle * 0.5f);
    const float fSpotParamScale = 1.0f / ezMath::Max(0.001f, (fCosInner - fCosOuter));
    const float fSpotParamOffset = -fCosOuter * fSpotParamScale;
    perLightData.spotParams = Float2ToRG16F(ezVec2(fSpotParamScale, fSpotParamOffset));
  }

  void FillDirLightData(ezPerLightData& perLightData, const ezDirectionalLightRenderData* pDirLightRenderData)
  {
    FillLightData(perLightData, pDirLightRenderData, LIGHT_TYPE_DIR);

    perLightData.direction = Float3ToRGB10(-pDirLightRenderData->m_GlobalTransform.m_Rotation.GetColumn(0));
  }

  template <typename Cluster>
  void RasterizeBox(const ezBoundingBox& box, const ezSimdMat4f& viewProjectionMatrix, const ezVec3& cameraPosition,
    ezUInt32 uiBlockIndex, ezUInt32 uiMask, ezArrayPtr<Cluster> clusters)
  {
    ezVec3 vCorners[8];
    box.GetCorners(vCorners);

    ezSimdBBox screenSpaceBBox;
    screenSpaceBBox.SetInvalid();

    for (ezUInt32 i = 0; i < 8; ++i)
    {
      ezSimdVec4f screenSpaceCorner = viewProjectionMatrix.TransformPosition(ezSimdConversion::ToVec4(vCorners[i].GetAsVec4(1.0)));
      ezSimdFloat w = screenSpaceCorner.w();
      screenSpaceCorner /= w;
      screenSpaceCorner.SetW(w);

      screenSpaceBBox.ExpandToInclude(screenSpaceCorner);
    }

    ezSimdVec4f scale;
    ezSimdVec4f bias;

    if (ezProjectionDepthRange::Default == ezProjectionDepthRange::ZeroToOne)
    {
      scale = ezSimdVec4f(0.5f, 0.5f, 1.0f, 1.0f);
      bias = ezSimdVec4f(0.5f, 0.5f, 0.0f, 0.0f);
    }
    else
    {
      scale = ezSimdVec4f(0.5f);
      bias = ezSimdVec4f(0.5f);
    }

    screenSpaceBBox.m_Min = ezSimdVec4f::MulAdd(screenSpaceBBox.m_Min, scale, bias);
    screenSpaceBBox.m_Max = ezSimdVec4f::MulAdd(screenSpaceBBox.m_Max, scale, bias);

    if (box.Contains(cameraPosition))
    {
      screenSpaceBBox.m_Min = ezSimdVec4f(0.0f).GetCombined<ezSwizzle::XYZW>(screenSpaceBBox.m_Min);
      screenSpaceBBox.m_Max = ezSimdVec4f(1.0f).GetCombined<ezSwizzle::XYZW>(screenSpaceBBox.m_Max);
    }

    ezUInt32 xMin = ezMath::Clamp((ezInt32)((float)screenSpaceBBox.m_Min.x() * NUM_CLUSTERS_X), 0, NUM_CLUSTERS_X - 1);
    ezUInt32 xMax = ezMath::Clamp((ezInt32)((float)screenSpaceBBox.m_Max.x() * NUM_CLUSTERS_X), 0, NUM_CLUSTERS_X - 1);

    ezUInt32 yMin = ezMath::Clamp((ezInt32)((1.0f - (float)screenSpaceBBox.m_Max.y()) * NUM_CLUSTERS_Y), 0, NUM_CLUSTERS_Y - 1);
    ezUInt32 yMax = ezMath::Clamp((ezInt32)((1.0f - (float)screenSpaceBBox.m_Min.y()) * NUM_CLUSTERS_Y), 0, NUM_CLUSTERS_Y - 1);

    ezUInt32 zMin = GetSliceIndexFromDepth(screenSpaceBBox.m_Min.w());
    ezUInt32 zMax = GetSliceIndexFromDepth(screenSpaceBBox.m_Max.w());

    for (ezUInt32 z = zMin; z <= zMax; ++z)
    {
      for (ezUInt32 y = yMin; y <= yMax; ++y)
      {
        for (ezUInt32 x = xMin; x <= xMax; ++x)
        {
          ezUInt32 uiClusterIndex = GetClusterIndexFromCoord(x, y, z);
          clusters[uiClusterIndex].m_BitMask[uiBlockIndex] |= uiMask;
        }
      }
    }
  }

  template <typename Cluster>
  void RasterizePointLight(const ezPointLightRenderData* pPointLightRenderData, ezUInt32 uiLightIndex, const ezSimdMat4f& viewProjectionMatrix,
    const ezVec3& cameraPosition, ezArrayPtr<Cluster> clusters)
  {
    ezBoundingBox box;
    box.SetCenterAndHalfExtents(pPointLightRenderData->m_GlobalTransform.m_vPosition, ezVec3(pPointLightRenderData->m_fRange));

    const ezUInt32 uiBlockIndex = uiLightIndex / 32;
    const ezUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    RasterizeBox(box, viewProjectionMatrix, cameraPosition, uiBlockIndex, uiMask, clusters);
  }

  template <typename Cluster>
  void RasterizeSpotLight(const ezSpotLightRenderData* pSpotLightRenderData, ezUInt32 uiLightIndex, const ezSimdMat4f& viewProjectionMatrix,
    const ezVec3& cameraPosition, ezArrayPtr<Cluster> clusters)
  {
    const float t = ezMath::Tan(pSpotLightRenderData->m_OuterSpotAngle * 0.5f);
    const float p = ezMath::Min(t * pSpotLightRenderData->m_fRange, pSpotLightRenderData->m_fRange);

    ezBoundingBox box;
    box.m_vMin = ezVec3(0.0f, -p, -p);
    box.m_vMax = ezVec3(pSpotLightRenderData->m_fRange, p, p);

    box.TransformFromOrigin(pSpotLightRenderData->m_GlobalTransform.GetAsMat4());

    const ezUInt32 uiBlockIndex = uiLightIndex / 32;
    const ezUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    RasterizeBox(box, viewProjectionMatrix, cameraPosition, uiBlockIndex, uiMask, clusters);
  }

  template <typename Cluster>
  void RasterizeDirLight(const ezDirectionalLightRenderData* pDirLightRenderData, ezUInt32 uiLightIndex, ezArrayPtr<Cluster> clusters)
  {
    const ezUInt32 uiBlockIndex = uiLightIndex / 32;
    const ezUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    for (ezUInt32 i = 0; i < clusters.GetCount(); ++i)
    {
      clusters[i].m_BitMask[uiBlockIndex] |= uiMask;
    }
  }
}
