#pragma once

#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>
EZ_DEFINE_AS_POD_TYPE(ezPerLightData);
EZ_DEFINE_AS_POD_TYPE(ezPerDecalData);
EZ_DEFINE_AS_POD_TYPE(ezPerClusterData);

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4i.h>

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

  // in order: tlf, trf, blf, brf, tln, trn, bln, brn
  EZ_FORCE_INLINE void GetClusterCornerPoints(const ezCamera& camera, float fZf, float fZn, float fTanFovX, float fTanFovY,
    ezInt32 x, ezInt32 y, ezInt32 z, ezVec3* out_pCorners)
  {
    const ezVec3& pos = camera.GetPosition();
    const ezVec3& dirForward = camera.GetDirForwards();
    const ezVec3& dirRight = camera.GetDirRight();
    const ezVec3& dirUp = camera.GetDirUp();

    float fStepXf = (fZf * fTanFovX) / NUM_CLUSTERS_X;
    float fStepYf = (fZf * fTanFovY) / NUM_CLUSTERS_Y;

    float fXf = (x - (NUM_CLUSTERS_X / 2)) * fStepXf;
    float fYf = (y - (NUM_CLUSTERS_Y / 2)) * fStepYf;

    out_pCorners[0] = pos + dirForward * fZf + dirRight * fXf - dirUp * fYf;
    out_pCorners[1] = out_pCorners[0] + dirRight * fStepXf;
    out_pCorners[2] = out_pCorners[0] - dirUp * fStepYf;
    out_pCorners[3] = out_pCorners[2] + dirRight * fStepXf;

    float fStepXn = (fZn * fTanFovX) / NUM_CLUSTERS_X;
    float fStepYn = (fZn * fTanFovY) / NUM_CLUSTERS_Y;
    float fXn = (x - (NUM_CLUSTERS_X / 2)) * fStepXn;
    float fYn = (y - (NUM_CLUSTERS_Y / 2)) * fStepYn;

    out_pCorners[4] = pos + dirForward * fZn + dirRight * fXn - dirUp * fYn;
    out_pCorners[5] = out_pCorners[4] + dirRight * fStepXn;
    out_pCorners[6] = out_pCorners[4] - dirUp * fStepYn;
    out_pCorners[7] = out_pCorners[6] + dirRight * fStepXn;
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

  void FillClusterBoundingSpheres(const ezCamera& camera, float fAspectRatio, ezArrayPtr<ezSimdBSphere> clusterBoundingSpheres)
  {
    ///\todo proper implementation for orthographic views
    if (camera.IsOrthographic())
      return;

    float fTanFovX = ezMath::Tan(camera.GetFovX(fAspectRatio) * 0.5f);
    float fTanFovY = ezMath::Tan(camera.GetFovY(fAspectRatio) * 0.5f);
    ezSimdVec4f fov = ezSimdVec4f(fTanFovX, fTanFovY, fTanFovX, fTanFovY);

    ezSimdVec4f pos = ezSimdConversion::ToVec3(camera.GetPosition());
    ezSimdVec4f dirForward = ezSimdConversion::ToVec3(camera.GetDirForwards());
    ezSimdVec4f dirRight = ezSimdConversion::ToVec3(camera.GetDirRight());
    ezSimdVec4f dirUp = ezSimdConversion::ToVec3(camera.GetDirUp());

    ezSimdVec4f numClusters = ezSimdVec4f(NUM_CLUSTERS_X, NUM_CLUSTERS_Y, NUM_CLUSTERS_X, NUM_CLUSTERS_Y);
    ezSimdVec4f halfNumClusters = numClusters * 0.5f;
    ezSimdVec4f stepScale = fov.CompDiv(halfNumClusters);

    ezSimdVec4f fZn = ezSimdVec4f::ZeroVector();
    ezSimdVec4f cc[8];

    for (ezInt32 z = 0; z < NUM_CLUSTERS_Z; z++)
    {
      ezSimdVec4f fZf = ezSimdVec4f(GetDepthFromSliceIndex(z));
      ezSimdVec4f zff_znn = fZf.GetCombined<ezSwizzle::XXXX>(fZn);
      ezSimdVec4f steps = zff_znn.CompMul(stepScale);

      ezSimdVec4f depthF = pos + dirForward * fZf.x();
      ezSimdVec4f depthN = pos + dirForward * fZn.x();

      for (ezInt32 y = 0; y < NUM_CLUSTERS_Y; y++)
      {
        for (ezInt32 x = 0; x < NUM_CLUSTERS_X; x++)
        {
          ezSimdVec4f xyxy = ezSimdVec4i(x, y, x, y).ToFloat();
          ezSimdVec4f xfyf = (xyxy - halfNumClusters).CompMul(steps);

          cc[0] = depthF + dirRight * xfyf.x() - dirUp * xfyf.y();
          cc[1] = cc[0] + dirRight * steps.x();
          cc[2] = cc[0] - dirUp * steps.y();
          cc[3] = cc[2] + dirRight * steps.x();

          cc[4] = depthN + dirRight * xfyf.z() - dirUp * xfyf.w();
          cc[5] = cc[4] + dirRight * steps.z();
          cc[6] = cc[4] - dirUp * steps.w();
          cc[7] = cc[6] + dirRight * steps.z();

          ezSimdBSphere s; s.SetFromPoints(cc, 8);

          clusterBoundingSpheres[GetClusterIndexFromCoord(x, y, z)] = s;
        }
      }

      fZn = fZf;
    }
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

    perLightData.direction = Float3ToRGB10(pSpotLightRenderData->m_GlobalTransform.m_qRotation * ezVec3(-1, 0, 0));
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

    perLightData.direction = Float3ToRGB10(pDirLightRenderData->m_GlobalTransform.m_qRotation * ezVec3(-1, 0, 0));
  }

  void FillDecalData(ezPerDecalData& perDecalData, const ezDecalRenderData* pDecalRenderData)
  {
    ezVec3 position = pDecalRenderData->m_GlobalTransform.m_vPosition;
    ezVec3 dirForwards = pDecalRenderData->m_GlobalTransform.m_qRotation * ezVec3(1.0f, 0.0, 0.0f);
    ezVec3 dirUp = pDecalRenderData->m_GlobalTransform.m_qRotation * ezVec3(0.0f, 0.0, 1.0f);
    ezVec3 scale = ezVec3(1.0f).CompDiv(pDecalRenderData->m_GlobalTransform.m_vScale.CompMul(pDecalRenderData->m_vHalfExtents));

    ezMat4 lookAt; lookAt.SetLookAtMatrix(position, position + dirForwards, dirUp);
    ezMat4 scaleMat; scaleMat.SetScalingMatrix(ezVec3(scale.y, -scale.z, scale.x));

    const float fCosInner = ezMath::Cos(pDecalRenderData->m_InnerFadeAngle);
    const float fCosOuter = ezMath::Cos(pDecalRenderData->m_OuterFadeAngle);
    const float fFadeParamScale = 1.0f / ezMath::Max(0.001f, (fCosInner - fCosOuter));
    const float fFadeParamOffset = -fCosOuter * fFadeParamScale;

    perDecalData.worldToDecalMatrix = scaleMat * lookAt;
    perDecalData.color = pDecalRenderData->m_Color;
    perDecalData.textureBitmask = 0;
    perDecalData.angleFadeParams = Float2ToRG16F(ezVec2(fFadeParamScale, fFadeParamOffset));
    perDecalData.baseAtlasScale = Float2ToRG16F(pDecalRenderData->m_vBaseAtlasScale);
    perDecalData.baseAtlasOffset = Float2ToRG16F(pDecalRenderData->m_vBaseAtlasOffset);
  }


  EZ_FORCE_INLINE ezSimdBBox GetScreenSpaceBounds(const ezSimdBSphere& sphere, const ezSimdMat4f& viewMatrix, const ezSimdMat4f& projectionMatrix)
  {
    ezSimdVec4f viewSpaceCenter = viewMatrix.TransformPosition(sphere.GetCenter());
    ezSimdFloat depth = viewSpaceCenter.z();
    ezSimdFloat radius = sphere.GetRadius();

    ezSimdVec4f mi;
    ezSimdVec4f ma;

    if (viewSpaceCenter.GetLength<3>() > radius && depth > radius)
    {
      ezSimdVec4f one = ezSimdVec4f(1.0f);
      ezSimdVec4f oneNegOne = ezSimdVec4f(1.0f, -1.0f, 1.0f, -1.0f);

      ezSimdVec4f pRadius = ezSimdVec4f(radius / depth);
      ezSimdVec4f pRadius2 = pRadius.CompMul(pRadius);

      ezSimdVec4f xy = viewSpaceCenter / depth;
      ezSimdVec4f xxyy = xy.Get<ezSwizzle::XXYY>();
      ezSimdVec4f nom = (pRadius2.CompMul(xxyy.CompMul(xxyy) - pRadius2 + one)).GetSqrt() - xxyy.CompMul(oneNegOne);
      ezSimdVec4f denom = pRadius2 - one;

      ezSimdVec4f projection = projectionMatrix.m_col0.GetCombined<ezSwizzle::XXYY>(projectionMatrix.m_col1);
      ezSimdVec4f minXmaxX_minYmaxY = nom.CompDiv(denom).CompMul(oneNegOne).CompMul(projection);

      mi = minXmaxX_minYmaxY.Get<ezSwizzle::XZXX>();
      ma = minXmaxX_minYmaxY.Get<ezSwizzle::YWYY>();
    }
    else
    {
      mi = ezSimdVec4f(-1.0f);
      ma = ezSimdVec4f(1.0f);
    }

    mi.SetZ(depth - radius);
    ma.SetZ(depth + radius);

    return ezSimdBBox(mi, ma);
  }

  template <typename Cluster, typename IntersectionFunc>
  EZ_FORCE_INLINE void FillCluster(const ezSimdBBox& screenSpaceBounds, ezUInt32 uiBlockIndex, ezUInt32 uiMask,
    Cluster* clusters, IntersectionFunc func)
  {
    ezSimdVec4f scale = ezSimdVec4f(0.5f * NUM_CLUSTERS_X, -0.5f * NUM_CLUSTERS_Y, 1.0f, 1.0f);
    ezSimdVec4f bias = ezSimdVec4f(0.5f * NUM_CLUSTERS_X, 0.5f * NUM_CLUSTERS_Y, 0.0f, 0.0f);

    ezSimdVec4f mi = ezSimdVec4f::MulAdd(screenSpaceBounds.m_Min, scale, bias);
    ezSimdVec4f ma = ezSimdVec4f::MulAdd(screenSpaceBounds.m_Max, scale, bias);

    ezSimdVec4i minXY_maxXY = ezSimdVec4i::Truncate(mi.GetCombined<ezSwizzle::XYXY>(ma));

    ezSimdVec4i maxClusterIndex = ezSimdVec4i(NUM_CLUSTERS_X, NUM_CLUSTERS_Y, NUM_CLUSTERS_X, NUM_CLUSTERS_Y);
    minXY_maxXY = minXY_maxXY.CompMin(maxClusterIndex - ezSimdVec4i(1));
    minXY_maxXY = minXY_maxXY.CompMax(ezSimdVec4i::ZeroVector());

    ezUInt32 xMin = minXY_maxXY.x();
    ezUInt32 yMin = minXY_maxXY.w();

    ezUInt32 xMax = minXY_maxXY.z();
    ezUInt32 yMax = minXY_maxXY.y();

    ezUInt32 zMin = GetSliceIndexFromDepth(screenSpaceBounds.m_Min.z());
    ezUInt32 zMax = GetSliceIndexFromDepth(screenSpaceBounds.m_Max.z());

    for (ezUInt32 z = zMin; z <= zMax; ++z)
    {
      for (ezUInt32 y = yMin; y <= yMax; ++y)
      {
        for (ezUInt32 x = xMin; x <= xMax; ++x)
        {
          ezUInt32 uiClusterIndex = GetClusterIndexFromCoord(x, y, z);
          if (func(uiClusterIndex))
          {
            clusters[uiClusterIndex].m_BitMask[uiBlockIndex] |= uiMask;
          }
        }
      }
    }
  }

  template <typename Cluster>
  void RasterizePointLight(const ezSimdBSphere& pointLightSphere, ezUInt32 uiLightIndex, const ezSimdMat4f& viewMatrix, const ezSimdMat4f& projectionMatrix,
    Cluster* clusters, ezSimdBSphere* clusterBoundingSpheres)
  {
    ezSimdBBox screenSpaceBounds = GetScreenSpaceBounds(pointLightSphere, viewMatrix, projectionMatrix);

    const ezUInt32 uiBlockIndex = uiLightIndex / 32;
    const ezUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, clusters, [&](ezUInt32 uiClusterIndex)
    {
      return pointLightSphere.Overlaps(clusterBoundingSpheres[uiClusterIndex]);
    });
  }

  struct BoundingCone
  {
    ezSimdBSphere m_BoundingSphere;
    ezSimdVec4f m_PositionAndRange;
    ezSimdVec4f m_ForwardDir;
    ezSimdVec4f m_SinCosAngle;
  };

  template <typename Cluster>
  void RasterizeSpotLight(const BoundingCone& spotLightCone, ezUInt32 uiLightIndex, const ezSimdMat4f& viewMatrix, const ezSimdMat4f& projectionMatrix,
    Cluster* clusters, ezSimdBSphere* clusterBoundingSpheres)
  {
    ezSimdVec4f position = spotLightCone.m_PositionAndRange;
    ezSimdFloat range = spotLightCone.m_PositionAndRange.w();
    ezSimdVec4f forwardDir = spotLightCone.m_ForwardDir;
    ezSimdFloat sinAngle = spotLightCone.m_SinCosAngle.x();
    ezSimdFloat cosAngle = spotLightCone.m_SinCosAngle.y();

    // First calculate a bounding sphere around the cone to get min and max bounds
    ezSimdVec4f bSphereCenter;
    ezSimdFloat bSphereRadius;
    if (sinAngle > 0.707107f) // sin(45)
    {
      bSphereCenter = position + forwardDir * cosAngle * range;
      bSphereRadius = sinAngle * range;
    }
    else
    {
      bSphereRadius = range / (cosAngle + cosAngle);
      bSphereCenter = position + forwardDir * bSphereRadius;
    }

    ezSimdBSphere spotLightSphere(bSphereCenter, bSphereRadius);
    ezSimdBBox screenSpaceBounds = GetScreenSpaceBounds(spotLightSphere, viewMatrix, projectionMatrix);

    const ezUInt32 uiBlockIndex = uiLightIndex / 32;
    const ezUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, clusters, [&](ezUInt32 uiClusterIndex)
    {
      ezSimdBSphere clusterSphere = clusterBoundingSpheres[uiClusterIndex];
      ezSimdFloat clusterRadius = clusterSphere.GetRadius();

      ezSimdVec4f toConePos = clusterSphere.m_CenterAndRadius - position;
      ezSimdFloat projected = forwardDir.Dot<3>(toConePos);
      ezSimdFloat distToConeSq = toConePos.Dot<3>(toConePos);
      ezSimdFloat distClosestP = cosAngle * (distToConeSq - projected * projected).GetSqrt() - projected * sinAngle;

      bool angleCull = distClosestP > clusterRadius;
      bool frontCull = projected > clusterRadius + range;
      bool backCull = projected < -clusterRadius;

      return !(angleCull || frontCull || backCull);
    });
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

  template <typename Cluster>
  void RasterizeDecal(const ezDecalRenderData* pDecalRenderData, ezUInt32 uiDecalIndex, const ezSimdMat4f& viewProjectionMatrix,
    Cluster* clusters, ezSimdBSphere* clusterBoundingSpheres)
  {
    ezSimdTransform decalToWorld = ezSimdConversion::ToTransform(pDecalRenderData->m_GlobalTransform);
    ezSimdTransform worldToDecal = decalToWorld.GetInverse();

    ezVec3 corners[8];
    ezBoundingBox(-pDecalRenderData->m_vHalfExtents, pDecalRenderData->m_vHalfExtents).GetCorners(corners);

    ezSimdMat4f decalToScreen = viewProjectionMatrix * decalToWorld.GetAsMat4();
    ezSimdBBox screenSpaceBounds; screenSpaceBounds.SetInvalid();
    bool bInsideBox = false;
    for (ezUInt32 i = 0; i < 8; ++i)
    {
      ezSimdVec4f corner = ezSimdConversion::ToVec3(corners[i]);
      ezSimdVec4f screenSpaceCorner = decalToScreen.TransformPosition(corner);
      ezSimdFloat depth = screenSpaceCorner.w();
      bInsideBox |= depth < ezSimdFloat::Zero();

      screenSpaceCorner /= depth;
      screenSpaceCorner = screenSpaceCorner.GetCombined<ezSwizzle::XYZW>(ezSimdVec4f(depth));

      screenSpaceBounds.m_Min = screenSpaceBounds.m_Min.CompMin(screenSpaceCorner);
      screenSpaceBounds.m_Max = screenSpaceBounds.m_Max.CompMax(screenSpaceCorner);
    }

    if (bInsideBox)
    {
      screenSpaceBounds.m_Min = ezSimdVec4f(-1.0f).GetCombined<ezSwizzle::XYZW>(screenSpaceBounds.m_Min);
      screenSpaceBounds.m_Max = ezSimdVec4f(1.0f).GetCombined<ezSwizzle::XYZW>(screenSpaceBounds.m_Max);
    }

    ezSimdVec4f decalHalfExtents = ezSimdConversion::ToVec3(pDecalRenderData->m_vHalfExtents);
    ezSimdBBox localDecalBounds = ezSimdBBox(-decalHalfExtents, decalHalfExtents);

    const ezUInt32 uiBlockIndex = uiDecalIndex / 32;
    const ezUInt32 uiMask = 1 << (uiDecalIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, clusters, [&](ezUInt32 uiClusterIndex)
    {
      ezSimdBSphere clusterSphere = clusterBoundingSpheres[uiClusterIndex];
      clusterSphere.Transform(worldToDecal);

      return localDecalBounds.Overlaps(clusterSphere);
    });
  }
}
