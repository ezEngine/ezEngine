#pragma once

#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/FillLightComponent.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>
EZ_DEFINE_AS_POD_TYPE(ezPerLightData);
EZ_DEFINE_AS_POD_TYPE(ezPerDecalData);
EZ_DEFINE_AS_POD_TYPE(ezPerReflectionProbeData);
EZ_DEFINE_AS_POD_TYPE(ezPerClusterData);

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/Utilities/GraphicsUtils.h>

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
  EZ_FORCE_INLINE void GetClusterCornerPoints(
    const ezCamera& camera, float fZf, float fZn, float fTanLeft, float fTanRight, float fTanBottom, float fTanTop, ezInt32 x, ezInt32 y, ezInt32 z, ezVec3* out_pCorners)
  {
    const ezVec3& pos = camera.GetPosition();
    const ezVec3& dirForward = camera.GetDirForwards();
    const ezVec3& dirRight = camera.GetDirRight();
    const ezVec3& dirUp = camera.GetDirUp();

    const float fStartXf = fZf * fTanLeft;
    const float fStartYf = fZf * fTanBottom;
    const float fEndXf = fZf * fTanRight;
    const float fEndYf = fZf * fTanTop;

    float fStepXf = (fEndXf - fStartXf) / NUM_CLUSTERS_X;
    float fStepYf = (fEndYf - fStartYf) / NUM_CLUSTERS_Y;

    float fXf = fStartXf + x * fStepXf;
    float fYf = fStartYf + y * fStepYf;

    out_pCorners[0] = pos + dirForward * fZf + dirRight * fXf - dirUp * fYf;
    out_pCorners[1] = out_pCorners[0] + dirRight * fStepXf;
    out_pCorners[2] = out_pCorners[0] - dirUp * fStepYf;
    out_pCorners[3] = out_pCorners[2] + dirRight * fStepXf;

    const float fStartXn = fZn * fTanLeft;
    const float fStartYn = fZn * fTanBottom;
    const float fEndXn = fZn * fTanRight;
    const float fEndYn = fZn * fTanTop;

    float fStepXn = (fEndXn - fStartXn) / NUM_CLUSTERS_X;
    float fStepYn = (fEndYn - fStartYn) / NUM_CLUSTERS_Y;
    float fXn = fStartXn + x * fStepXn;
    float fYn = fStartYn + y * fStepYn;

    out_pCorners[4] = pos + dirForward * fZn + dirRight * fXn - dirUp * fYn;
    out_pCorners[5] = out_pCorners[4] + dirRight * fStepXn;
    out_pCorners[6] = out_pCorners[4] - dirUp * fStepYn;
    out_pCorners[7] = out_pCorners[6] + dirRight * fStepXn;
  }

  void FillClusterBoundingSpheres(const ezCamera& camera, float fAspectRatio, ezArrayPtr<ezSimdBSphere> clusterBoundingSpheres)
  {
    EZ_PROFILE_SCOPE("FillClusterBoundingSpheres");

    ///\todo proper implementation for orthographic views
    if (camera.IsOrthographic())
      return;

    ezMat4 mProj;
    camera.GetProjectionMatrix(fAspectRatio, mProj);

    ezSimdVec4f stepScale;
    ezSimdVec4f tanLBLB;
    {
      ezAngle fFovLeft;
      ezAngle fFovRight;
      ezAngle fFovBottom;
      ezAngle fFovTop;
      ezGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(mProj, fFovLeft, fFovRight, fFovBottom, fFovTop);

      const float fTanLeft = ezMath::Tan(fFovLeft);
      const float fTanRight = ezMath::Tan(fFovRight);
      const float fTanBottom = ezMath::Tan(fFovBottom);
      const float fTanTop = ezMath::Tan(fFovTop);

      float fStepXf = (fTanRight - fTanLeft) / NUM_CLUSTERS_X;
      float fStepYf = (fTanTop - fTanBottom) / NUM_CLUSTERS_Y;

      stepScale = ezSimdVec4f(fStepXf, fStepYf, fStepXf, fStepYf);
      tanLBLB = ezSimdVec4f(fTanLeft, fTanBottom, fTanLeft, fTanBottom);
    }

    ezSimdVec4f pos = ezSimdConversion::ToVec3(camera.GetPosition());
    ezSimdVec4f dirForward = ezSimdConversion::ToVec3(camera.GetDirForwards());
    ezSimdVec4f dirRight = ezSimdConversion::ToVec3(camera.GetDirRight());
    ezSimdVec4f dirUp = ezSimdConversion::ToVec3(camera.GetDirUp());


    ezSimdVec4f fZn = ezSimdVec4f::MakeZero();
    ezSimdVec4f cc[8];

    for (ezInt32 z = 0; z < NUM_CLUSTERS_Z; z++)
    {
      ezSimdVec4f fZf = ezSimdVec4f(GetDepthFromSliceIndex(z));
      ezSimdVec4f zff_znn = fZf.GetCombined<ezSwizzle::XXXX>(fZn);
      ezSimdVec4f steps = zff_znn.CompMul(stepScale);

      ezSimdVec4f depthF = pos + dirForward * fZf.x();
      ezSimdVec4f depthN = pos + dirForward * fZn.x();

      ezSimdVec4f startLBLB = zff_znn.CompMul(tanLBLB);

      for (ezInt32 y = 0; y < NUM_CLUSTERS_Y; y++)
      {
        for (ezInt32 x = 0; x < NUM_CLUSTERS_X; x++)
        {
          ezSimdVec4f xyxy = ezSimdVec4i(x, y, x, y).ToFloat();
          ezSimdVec4f xfyf = startLBLB + (xyxy).CompMul(steps);

          cc[0] = depthF + dirRight * xfyf.x() - dirUp * xfyf.y();
          cc[1] = cc[0] + dirRight * steps.x();
          cc[2] = cc[0] - dirUp * steps.y();
          cc[3] = cc[2] + dirRight * steps.x();

          cc[4] = depthN + dirRight * xfyf.z() - dirUp * xfyf.w();
          cc[5] = cc[4] + dirRight * steps.z();
          cc[6] = cc[4] - dirUp * steps.w();
          cc[7] = cc[6] + dirRight * steps.z();

          clusterBoundingSpheres[GetClusterIndexFromCoord(x, y, z)] = ezSimdBSphere::MakeFromPoints(cc, 8);
        }
      }

      fZn = fZf;
    }
  }

  EZ_ALWAYS_INLINE void FillLightData(ezPerLightData& out_perLightData, const ezLightRenderData* pLightRenderData, ezUInt8 uiType)
  {
    ezMemoryUtils::ZeroFill(&out_perLightData, 1);

    ezColorLinearUB lightColor = pLightRenderData->m_LightColor;
    lightColor.a = uiType;

    out_perLightData.colorAndType = *reinterpret_cast<ezUInt32*>(&lightColor.r);
    out_perLightData.intensity = pLightRenderData->m_fIntensity;
    out_perLightData.specularMultiplier = pLightRenderData->m_fSpecularMultiplier;
    out_perLightData.shadowDataOffsetAndFadeOut = pLightRenderData->m_uiShadowDataOffsetAndFadeOut;
  }

  void FillPointLightData(ezPerLightData& out_perLightData, const ezPointLightRenderData* pPointLightRenderData)
  {
    FillLightData(out_perLightData, pPointLightRenderData, LIGHT_TYPE_POINT);

    out_perLightData.position = pPointLightRenderData->m_GlobalTransform.m_vPosition;
    out_perLightData.invSqrAttRadius = 1.0f / (pPointLightRenderData->m_fRange * pPointLightRenderData->m_fRange);
  }

  void FillSpotLightData(ezPerLightData& out_perLightData, const ezSpotLightRenderData* pSpotLightRenderData)
  {
    FillLightData(out_perLightData, pSpotLightRenderData, LIGHT_TYPE_SPOT);

    out_perLightData.direction = ezShaderUtils::Float3ToRGB10(pSpotLightRenderData->m_GlobalTransform.m_qRotation * ezVec3(-1, 0, 0));
    out_perLightData.position = pSpotLightRenderData->m_GlobalTransform.m_vPosition;
    out_perLightData.invSqrAttRadius = 1.0f / (pSpotLightRenderData->m_fRange * pSpotLightRenderData->m_fRange);

    const float fCosInner = ezMath::Cos(pSpotLightRenderData->m_InnerSpotAngle * 0.5f);
    const float fCosOuter = ezMath::Cos(pSpotLightRenderData->m_OuterSpotAngle * 0.5f);
    const float fSpotParamScale = 1.0f / ezMath::Max(0.001f, (fCosInner - fCosOuter));
    const float fSpotParamOffset = -fCosOuter * fSpotParamScale;
    out_perLightData.spotOrFillParams = ezShaderUtils::Float2ToRG16F(ezVec2(fSpotParamScale, fSpotParamOffset));
  }

  void FillDirLightData(ezPerLightData& out_perLightData, const ezDirectionalLightRenderData* pDirLightRenderData)
  {
    FillLightData(out_perLightData, pDirLightRenderData, LIGHT_TYPE_DIR);

    out_perLightData.direction = ezShaderUtils::Float3ToRGB10(pDirLightRenderData->m_GlobalTransform.m_qRotation * ezVec3(-1, 0, 0));
  }

  void FillFillLightData(ezPerLightData& out_perLightData, const ezFillLightRenderData* pFillLightRenderData)
  {
    ezMemoryUtils::ZeroFill(&out_perLightData, 1);

    ezColorLinearUB lightColor = pFillLightRenderData->m_LightColor;
    out_perLightData.intensity = pFillLightRenderData->m_fIntensity;

    switch (pFillLightRenderData->m_LightMode)
    {
      case ezFillLightMode::Additive:
        lightColor.a = LIGHT_TYPE_FILL_ADDITIVE;
        break;
      case ezFillLightMode::Subtractive:
        lightColor.a = LIGHT_TYPE_FILL_ADDITIVE;
        out_perLightData.intensity = -out_perLightData.intensity;
        break;
      case ezFillLightMode::ModulateIndirect:
        lightColor.a = LIGHT_TYPE_FILL_MODULATE_INDIRECT;
        out_perLightData.intensity = ezMath::Saturate(out_perLightData.intensity);
        break;
    }

    out_perLightData.colorAndType = *reinterpret_cast<ezUInt32*>(&lightColor.r);
    out_perLightData.specularMultiplier = 0.0f; // no specular for fill lights

    out_perLightData.position = pFillLightRenderData->m_GlobalTransform.m_vPosition;
    out_perLightData.invSqrAttRadius = 1.0f / pFillLightRenderData->m_fRange;

    const float fFalloffExponent = ezMath::Max(pFillLightRenderData->m_fFalloffExponent, 0.001f);
    out_perLightData.spotOrFillParams = ezShaderUtils::Float2ToRG16F(ezVec2(fFalloffExponent, pFillLightRenderData->m_fDirectionality));
  }

  void FillDecalData(ezPerDecalData& out_perDecalData, const ezDecalRenderData* pDecalRenderData)
  {
    ezVec3 position = pDecalRenderData->m_GlobalTransform.m_vPosition;
    ezVec3 dirForwards = pDecalRenderData->m_GlobalTransform.m_qRotation * ezVec3(1.0f, 0.0, 0.0f);
    ezVec3 dirUp = pDecalRenderData->m_GlobalTransform.m_qRotation * ezVec3(0.0f, 0.0, 1.0f);
    ezVec3 scale = pDecalRenderData->m_GlobalTransform.m_vScale;

    // the CompMax prevents division by zero (thus inf, thus NaN later, then crash)
    // if negative scaling should be allowed, this would need to be changed
    scale = ezVec3(1.0f).CompDiv(scale.CompMax(ezVec3(0.00001f)));

    const ezMat4 lookAt = ezGraphicsUtils::CreateLookAtViewMatrix(position, position + dirForwards, dirUp);
    ezMat4 scaleMat = ezMat4::MakeScaling(ezVec3(scale.y, -scale.z, scale.x));

    out_perDecalData.worldToDecalMatrix = scaleMat * lookAt;
    out_perDecalData.applyOnlyToId = pDecalRenderData->m_uiApplyOnlyToId;
    out_perDecalData.decalFlags = pDecalRenderData->m_uiFlags;
    out_perDecalData.angleFadeParams = pDecalRenderData->m_uiAngleFadeParams;
    out_perDecalData.baseColor = *reinterpret_cast<const ezUInt32*>(&pDecalRenderData->m_BaseColor.r);
    out_perDecalData.emissiveColorRG = ezShaderUtils::PackFloat16intoUint(pDecalRenderData->m_EmissiveColor.r, pDecalRenderData->m_EmissiveColor.g);
    out_perDecalData.emissiveColorBA = ezShaderUtils::PackFloat16intoUint(pDecalRenderData->m_EmissiveColor.b, pDecalRenderData->m_EmissiveColor.a);
    out_perDecalData.baseColorAtlasScale = pDecalRenderData->m_uiBaseColorAtlasScale;
    out_perDecalData.baseColorAtlasOffset = pDecalRenderData->m_uiBaseColorAtlasOffset;
    out_perDecalData.normalAtlasScale = pDecalRenderData->m_uiNormalAtlasScale;
    out_perDecalData.normalAtlasOffset = pDecalRenderData->m_uiNormalAtlasOffset;
    out_perDecalData.ormAtlasScale = pDecalRenderData->m_uiORMAtlasScale;
    out_perDecalData.ormAtlasOffset = pDecalRenderData->m_uiORMAtlasOffset;
  }

  void FillReflectionProbeData(ezPerReflectionProbeData& out_perReflectionProbeData, const ezReflectionProbeRenderData* pReflectionProbeRenderData)
  {
    ezVec3 position = pReflectionProbeRenderData->m_GlobalTransform.m_vPosition;
    ezVec3 scale = pReflectionProbeRenderData->m_GlobalTransform.m_vScale.CompMul(pReflectionProbeRenderData->m_vHalfExtents);

    // We store scale separately so we easily transform into probe projection space (with scale), influence space (scale + offset) and cube map space (no scale).
    auto trans = pReflectionProbeRenderData->m_GlobalTransform;
    trans.m_vScale = ezVec3(1.0f, 1.0f, 1.0f);
    auto inverse = trans.GetAsMat4().GetInverse();

    // the CompMax prevents division by zero (thus inf, thus NaN later, then crash)
    // if negative scaling should be allowed, this would need to be changed
    scale = ezVec3(1.0f).CompDiv(scale.CompMax(ezVec3(0.00001f)));
    out_perReflectionProbeData.WorldToProbeProjectionMatrix = inverse;

    out_perReflectionProbeData.ProbePosition = pReflectionProbeRenderData->m_vProbePosition.GetAsVec4(1.0f); // W isn't used.
    out_perReflectionProbeData.Scale = scale.GetAsVec4(0.0f);                                                // W isn't used.

    out_perReflectionProbeData.InfluenceScale = pReflectionProbeRenderData->m_vInfluenceScale.GetAsVec4(0.0f);
    out_perReflectionProbeData.InfluenceShift = pReflectionProbeRenderData->m_vInfluenceShift.CompMul(ezVec3(1.0f) - pReflectionProbeRenderData->m_vInfluenceScale).GetAsVec4(0.0f);

    out_perReflectionProbeData.PositiveFalloff = pReflectionProbeRenderData->m_vPositiveFalloff.GetAsVec4(0.0f);
    out_perReflectionProbeData.NegativeFalloff = pReflectionProbeRenderData->m_vNegativeFalloff.GetAsVec4(0.0f);
    out_perReflectionProbeData.Index = pReflectionProbeRenderData->m_uiIndex;
  }


  EZ_FORCE_INLINE ezSimdBBox GetScreenSpaceBounds(const ezSimdBSphere& sphere, const ezSimdMat4f& mViewMatrix, const ezSimdMat4f& mProjectionMatrix)
  {
    ezSimdVec4f viewSpaceCenter = mViewMatrix.TransformPosition(sphere.GetCenter());
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

      ezSimdVec4f projection = mProjectionMatrix.m_col0.GetCombined<ezSwizzle::XXYY>(mProjectionMatrix.m_col1);
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
  EZ_FORCE_INLINE void FillCluster(const ezSimdBBox& screenSpaceBounds, ezUInt32 uiBlockIndex, ezUInt32 uiMask, Cluster* pClusters, IntersectionFunc func)
  {
    ezSimdVec4f scale = ezSimdVec4f(0.5f * NUM_CLUSTERS_X, -0.5f * NUM_CLUSTERS_Y, 1.0f, 1.0f);
    ezSimdVec4f bias = ezSimdVec4f(0.5f * NUM_CLUSTERS_X, 0.5f * NUM_CLUSTERS_Y, 0.0f, 0.0f);

    ezSimdVec4f mi = ezSimdVec4f::MulAdd(screenSpaceBounds.m_Min, scale, bias);
    ezSimdVec4f ma = ezSimdVec4f::MulAdd(screenSpaceBounds.m_Max, scale, bias);

    ezSimdVec4i minXY_maxXY = ezSimdVec4i::Truncate(mi.GetCombined<ezSwizzle::XYXY>(ma));

    ezSimdVec4i maxClusterIndex = ezSimdVec4i(NUM_CLUSTERS_X, NUM_CLUSTERS_Y, NUM_CLUSTERS_X, NUM_CLUSTERS_Y);
    minXY_maxXY = minXY_maxXY.CompMin(maxClusterIndex - ezSimdVec4i(1));
    minXY_maxXY = minXY_maxXY.CompMax(ezSimdVec4i::MakeZero());

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
            pClusters[uiClusterIndex].m_BitMask[uiBlockIndex] |= uiMask;
          }
        }
      }
    }
  }

  template <typename Cluster>
  void RasterizeSphere(const ezSimdBSphere& pointLightSphere, ezUInt32 uiLightIndex, const ezSimdMat4f& mViewMatrix,
    const ezSimdMat4f& mProjectionMatrix, Cluster* pClusters, ezSimdBSphere* pClusterBoundingSpheres)
  {
    ezSimdBBox screenSpaceBounds = GetScreenSpaceBounds(pointLightSphere, mViewMatrix, mProjectionMatrix);

    const ezUInt32 uiBlockIndex = uiLightIndex / 32;
    const ezUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, pClusters,
      [&](ezUInt32 uiClusterIndex)
      { return pointLightSphere.Overlaps(pClusterBoundingSpheres[uiClusterIndex]); });
  }

  struct BoundingCone
  {
    ezSimdBSphere m_BoundingSphere;
    ezSimdVec4f m_PositionAndRange;
    ezSimdVec4f m_ForwardDir;
    ezSimdVec4f m_SinCosAngle;
  };

  template <typename Cluster>
  void RasterizeSpotLight(const BoundingCone& spotLightCone, ezUInt32 uiLightIndex, const ezSimdMat4f& mViewMatrix,
    const ezSimdMat4f& mProjectionMatrix, Cluster* pClusters, ezSimdBSphere* pClusterBoundingSpheres)
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
    ezSimdBBox screenSpaceBounds = GetScreenSpaceBounds(spotLightSphere, mViewMatrix, mProjectionMatrix);

    const ezUInt32 uiBlockIndex = uiLightIndex / 32;
    const ezUInt32 uiMask = 1 << (uiLightIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, pClusters, [&](ezUInt32 uiClusterIndex)
      {
      ezSimdBSphere clusterSphere = pClusterBoundingSpheres[uiClusterIndex];
      ezSimdFloat clusterRadius = clusterSphere.GetRadius();

      ezSimdVec4f toConePos = clusterSphere.m_CenterAndRadius - position;
      ezSimdFloat projected = forwardDir.Dot<3>(toConePos);
      ezSimdFloat distToConeSq = toConePos.Dot<3>(toConePos);
      ezSimdFloat distClosestP = cosAngle * (distToConeSq - projected * projected).GetSqrt() - projected * sinAngle;

      bool angleCull = distClosestP > clusterRadius;
      bool frontCull = projected > clusterRadius + range;
      bool backCull = projected < -clusterRadius;

      return !(angleCull || frontCull || backCull); });
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
  void RasterizeBox(const ezTransform& transform, ezUInt32 uiDecalIndex, const ezSimdMat4f& mViewProjectionMatrix, Cluster* pClusters,
    ezSimdBSphere* pClusterBoundingSpheres)
  {
    ezSimdMat4f decalToWorld = ezSimdConversion::ToTransform(transform).GetAsMat4();
    ezSimdMat4f worldToDecal = decalToWorld.GetInverse();

    ezVec3 corners[8];
    ezBoundingBox::MakeFromMinMax(ezVec3(-1), ezVec3(1)).GetCorners(corners);

    ezSimdMat4f decalToScreen = mViewProjectionMatrix * decalToWorld;
    ezSimdBBox screenSpaceBounds = ezSimdBBox::MakeInvalid();
    bool bInsideBox = false;
    for (ezUInt32 i = 0; i < 8; ++i)
    {
      ezSimdVec4f corner = ezSimdConversion::ToVec3(corners[i]);
      ezSimdVec4f screenSpaceCorner = decalToScreen.TransformPosition(corner);
      ezSimdFloat depth = screenSpaceCorner.w();
      bInsideBox |= depth < ezSimdFloat::MakeZero();

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

    ezSimdVec4f decalHalfExtents = ezSimdVec4f(1.0f);
    ezSimdBBox localDecalBounds = ezSimdBBox(-decalHalfExtents, decalHalfExtents);

    const ezUInt32 uiBlockIndex = uiDecalIndex / 32;
    const ezUInt32 uiMask = 1 << (uiDecalIndex - uiBlockIndex * 32);

    FillCluster(screenSpaceBounds, uiBlockIndex, uiMask, pClusters, [&](ezUInt32 uiClusterIndex)
      {
      ezSimdBSphere clusterSphere = pClusterBoundingSpheres[uiClusterIndex];
      clusterSphere.Transform(worldToDecal);

      return localDecalBounds.Overlaps(clusterSphere); });
  }
} // namespace
