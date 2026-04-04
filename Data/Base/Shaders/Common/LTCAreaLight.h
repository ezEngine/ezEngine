#pragma once

#include "BRDF.h"
#include "LightData.h"

Texture2D LtcMatTexture;
Texture2D LtcMagTexture;
SamplerState LtcSampler;

static const float LTC_LUT_SIZE = 64.0;
static const float LTC_LUT_SCALE = (LTC_LUT_SIZE - 1.0) / LTC_LUT_SIZE;
static const float LTC_LUT_BIAS = 0.5 / LTC_LUT_SIZE;
static const float LTC_NAN_EPSILON = 1e-7;
static const float LTC_MAX_RESULT = 1e4;

float3 LTC_SafeNormalize(float3 v)
{
  float lenSq = dot(v, v);
  return lenSq > LTC_NAN_EPSILON ? v * rsqrt(lenSq) : float3(0, 0, 0);
}

float LTC_Sanitize(float x)
{
  return (x >= 0.0 && x <= LTC_MAX_RESULT) ? x : 0.0;
}

float3 LTC_BuildTangent(float3 V, float3 N)
{
  float3 projected = V - N * dot(V, N);
  float lenSq = dot(projected, projected);
  if (lenSq > LTC_NAN_EPSILON)
    return projected * rsqrt(lenSq);
  float3 alt = abs(N.x) < 0.9 ? float3(1, 0, 0) : float3(0, 1, 0);
  return normalize(alt - N * dot(alt, N));
}

float3 LTC_IntegrateEdgeVec(float3 v1, float3 v2)
{
  float x = dot(v1, v2);
  float y = abs(x);

  float a = 0.8543985 + (0.4965155 + 0.0145206 * y) * y;
  float b = 3.4175940 + (4.1616724 + y) * y;
  float v = a / b;

  float theta_sintheta = (x > 0.0) ? v : 0.5 * rsqrt(max(1.0 - x * x, 1e-7)) - v;

  return cross(v1, v2) * theta_sintheta;
}

// Computes the dominant UV on the rect from LTC-transformed (pre-normalization) corner points.
// Projects the origin (shading point) onto the transformed quad plane and finds parametric coords.
// Based on CryEngine's FetchDiffuseFilteredTexture approach.
float2 ComputeLTCDominantUV(float3 L0, float3 L1, float3 L2, float3 L3)
{
  float3 V1 = L1 - L0;
  float3 V2 = L3 - L0;
  float3 planeOrtho = cross(V1, V2);
  float planeAreaSquared = dot(planeOrtho, planeOrtho);
  float planeDistxPlaneArea = dot(planeOrtho, L0);

  float3 P = planeDistxPlaneArea * planeOrtho / max(planeAreaSquared, 1e-7) - L0;

  float dot_V1_V2 = dot(V1, V2);
  float inv_dot_V1_V1 = 1.0 / max(dot(V1, V1), 1e-7);
  float3 V2_ = V2 - V1 * dot_V1_V2 * inv_dot_V1_V1;
  float2 Puv;
  Puv.y = dot(V2_, P) / max(dot(V2_, V2_), 1e-7);
  Puv.x = dot(V1, P) * inv_dot_V1_V1 - dot_V1_V2 * inv_dot_V1_V1 * Puv.y;

  return Puv;
}

AccumulatedLight RectLightShading(ezPerLightData lightData, ezMaterialData matData, float3 V,
  out float2 outDiffUV, out float2 outSpecUV)
{
  outDiffUV = float2(0.5, 0.5);
  outSpecUV = float2(0.5, 0.5);

  float3 N = matData.worldNormal;
  float3 P = matData.worldPosition;

  float NdotV = saturate(dot(N, V));

  float2 uv = float2(matData.roughness, sqrt(1.0 - NdotV));
  uv = uv * LTC_LUT_SCALE + LTC_LUT_BIAS;

  float4 t1 = LtcMatTexture.SampleLevel(LtcSampler, uv, 0);
  float4 t2 = LtcMagTexture.SampleLevel(LtcSampler, uv, 0);

  float3x3 Minv = float3x3(
    float3(t1.x, 0, t1.z),
    float3(  0,  1,   0 ),
    float3(t1.y, 0, t1.w)
  );

  float3 lightPos = lightData.position;
  float3 lightForward = GetLightDirection(lightData);
  float3 lightRight = GetRectRightDirection(lightData);
  float3 lightUp = cross(lightRight, lightForward);
  float2 halfSize = GetRectHalfSize(lightData);
  bool twoSided = lightData.twoSided > 0.5;

  if (!twoSided && dot(P - lightPos, lightForward) < 0)
    return InitializeLight(0, 0);

  float3 rectPoints[4];
  rectPoints[0] = lightPos - lightRight * halfSize.x - lightUp * halfSize.y;
  rectPoints[1] = lightPos + lightRight * halfSize.x - lightUp * halfSize.y;
  rectPoints[2] = lightPos + lightRight * halfSize.x + lightUp * halfSize.y;
  rectPoints[3] = lightPos - lightRight * halfSize.x + lightUp * halfSize.y;

  float3 T1 = LTC_BuildTangent(V, N);
  float3 T2 = cross(N, T1);
  float3x3 onb = float3x3(T1, T2, N);

  // === Specular (LTC-warped) ===
  float3x3 Mspec = mul(Minv, onb);
  float3 Lspec[4];
  Lspec[0] = mul(Mspec, rectPoints[0] - P);
  Lspec[1] = mul(Mspec, rectPoints[1] - P);
  Lspec[2] = mul(Mspec, rectPoints[2] - P);
  Lspec[3] = mul(Mspec, rectPoints[3] - P);

  outSpecUV = saturate(ComputeLTCDominantUV(Lspec[0], Lspec[1], Lspec[2], Lspec[3]));

  Lspec[0] = LTC_SafeNormalize(Lspec[0]);
  Lspec[1] = LTC_SafeNormalize(Lspec[1]);
  Lspec[2] = LTC_SafeNormalize(Lspec[2]);
  Lspec[3] = LTC_SafeNormalize(Lspec[3]);

  float3 vsumSpec = float3(0, 0, 0);
  vsumSpec += LTC_IntegrateEdgeVec(Lspec[0], Lspec[1]);
  vsumSpec += LTC_IntegrateEdgeVec(Lspec[1], Lspec[2]);
  vsumSpec += LTC_IntegrateEdgeVec(Lspec[2], Lspec[3]);
  vsumSpec += LTC_IntegrateEdgeVec(Lspec[3], Lspec[0]);

  float specular = twoSided ? abs(vsumSpec.z) : max(0.0, vsumSpec.z);

  // === Diffuse (identity transform) ===
  float3 Ldiff[4];
  Ldiff[0] = mul(onb, rectPoints[0] - P);
  Ldiff[1] = mul(onb, rectPoints[1] - P);
  Ldiff[2] = mul(onb, rectPoints[2] - P);
  Ldiff[3] = mul(onb, rectPoints[3] - P);

  outDiffUV = ComputeLTCDominantUV(Ldiff[0], Ldiff[1], Ldiff[2], Ldiff[3]);

  Ldiff[0] = LTC_SafeNormalize(Ldiff[0]);
  Ldiff[1] = LTC_SafeNormalize(Ldiff[1]);
  Ldiff[2] = LTC_SafeNormalize(Ldiff[2]);
  Ldiff[3] = LTC_SafeNormalize(Ldiff[3]);

  float3 vsumDiff = float3(0, 0, 0);
  vsumDiff += LTC_IntegrateEdgeVec(Ldiff[0], Ldiff[1]);
  vsumDiff += LTC_IntegrateEdgeVec(Ldiff[1], Ldiff[2]);
  vsumDiff += LTC_IntegrateEdgeVec(Ldiff[2], Ldiff[3]);
  vsumDiff += LTC_IntegrateEdgeVec(Ldiff[3], Ldiff[0]);

  float diffuse = twoSided ? abs(vsumDiff.z) : max(0.0, vsumDiff.z);

  float3 F = matData.specularColor * t2.x + (1.0 - matData.specularColor) * t2.y;

  return InitializeLight(matData.diffuseColor * LTC_Sanitize(diffuse), F * LTC_Sanitize(specular));
}

// === Disk Light (LTC ellipse projection) ===
// Based on "Real-Time Area Lighting: a Journey from Research to Production" (Heitz et al.)
// and the reference ltcDisk.frag implementation.

float3 SolveCubic(float4 Coefficient)
{
  Coefficient.xyz /= Coefficient.w;
  Coefficient.yz /= 3.0;

  float B = Coefficient.z;
  float C = Coefficient.y;
  float D = Coefficient.x;

  float3 Delta = float3(
    -B * B + C,
    -C * B + D,
    B * C - B * B * B - D + B * C // dot(float2(B, -C), float2(C, D))
  );
  Delta.z = dot(float2(B, -C), Coefficient.xy);

  float Discriminant = dot(float2(4.0 * Delta.x, -Delta.y), Delta.zy);

  float2 xlc, xsc;

  {
    float C_a = Delta.x;
    float D_a = -2.0 * B * Delta.x + Delta.y;

    float Theta = atan2(sqrt(max(Discriminant, 0.0)), -D_a) / 3.0;
    float sqrtNegCa = sqrt(max(-C_a, 0.0));
    float x_1a = 2.0 * sqrtNegCa * cos(Theta);
    float x_3a = 2.0 * sqrtNegCa * cos(Theta + (2.0 / 3.0) * PI);

    float xl = ((x_1a + x_3a) > 2.0 * B) ? x_1a : x_3a;
    xlc = float2(xl - B, 1.0);
  }

  {
    float C_d = Delta.z;
    float D_d = -D * Delta.y + 2.0 * C * Delta.z;

    float Theta = atan2(D * sqrt(max(Discriminant, 0.0)), -D_d) / 3.0;
    float sqrtNegCd = sqrt(max(-C_d, 0.0));
    float x_1d = 2.0 * sqrtNegCd * cos(Theta);
    float x_3d = 2.0 * sqrtNegCd * cos(Theta + (2.0 / 3.0) * PI);

    float xs = (x_1d + x_3d < 2.0 * C) ? x_1d : x_3d;
    xsc = float2(-D, xs + C);
  }

  float E = xlc.y * xsc.y;
  float F2 = -xlc.x * xsc.y - xlc.y * xsc.x;
  float G = xlc.x * xsc.x;

  float2 xmc = float2(C * F2 - B * G, -B * F2 + C * E);

  float3 Root = float3(xsc.x / xsc.y, xmc.x / xmc.y, xlc.x / xlc.y);

  if (Root.x < Root.y && Root.x < Root.z)
    Root.xyz = Root.yxz;
  else if (Root.z < Root.x && Root.z < Root.y)
    Root.xyz = Root.xzy;

  return Root;
}

AccumulatedLight DiskLightShading(ezPerLightData lightData, ezMaterialData matData, float3 V)
{
  float3 N = matData.worldNormal;
  float3 P = matData.worldPosition;

  float NdotV = saturate(dot(N, V));

  float2 uv = float2(matData.roughness, sqrt(1.0 - NdotV));
  uv = uv * LTC_LUT_SCALE + LTC_LUT_BIAS;

  float4 t1 = LtcMatTexture.SampleLevel(LtcSampler, uv, 0);
  float4 t2 = LtcMagTexture.SampleLevel(LtcSampler, uv, 0);

  float3x3 Minv = float3x3(
    float3(t1.x, 0, t1.z),
    float3(  0,  1,   0 ),
    float3(t1.y, 0, t1.w)
  );

  float3 lightPos = lightData.position;
  float3 lightForward = GetLightDirection(lightData);
  float3 lightRight = GetCylinderAxisDirection(lightData);
  float3 lightUp = cross(lightRight, lightForward);
  float diskRadius = GetDiskRadius(lightData);
  bool twoSided = lightData.twoSided > 0.5;

  if (!twoSided && dot(P - lightPos, lightForward) < 0)
    return InitializeLight(0, 0);

  float3 diskPoints[3];
  diskPoints[0] = lightPos + lightRight * diskRadius;
  diskPoints[1] = lightPos + lightUp * diskRadius;
  diskPoints[2] = lightPos - lightRight * diskRadius;

  float3 T1 = LTC_BuildTangent(V, N);
  float3 T2 = cross(N, T1);
  float3x3 onb = float3x3(T1, T2, N);

  float diffuse = 0;
  float specular = 0;

  [unroll]
  for (int i = 0; i < 2; i++)
  {
    float3x3 M = (i == 0) ? mul(Minv, onb) : onb;

    float3 L_[3];
    L_[0] = mul(M, diskPoints[0] - P);
    L_[1] = mul(M, diskPoints[1] - P);
    L_[2] = mul(M, diskPoints[2] - P);

    float3 C_ = 0.5 * (L_[0] + L_[2]);
    float3 V1_ = 0.5 * (L_[1] - L_[2]);
    float3 V2_ = 0.5 * (L_[1] - L_[0]);

    if (!twoSided && dot(cross(V1_, V2_), C_) < 0.0)
    {
      if (i == 0) specular = 0;
      else diffuse = 0;
      continue;
    }

    float d11 = dot(V1_, V1_);
    float d22 = dot(V2_, V2_);
    float d12 = dot(V1_, V2_);

    if (d11 < LTC_NAN_EPSILON && d22 < LTC_NAN_EPSILON)
    {
      if (i == 0) specular = 0;
      else diffuse = 0;
      continue;
    }

    float a, b;
    if (abs(d12) / sqrt(max(d11 * d22, 1e-7)) > 0.0001)
    {
      float tr = d11 + d22;
      float det2 = sqrt(max(-d12 * d12 + d11 * d22, 0.0));
      float u = 0.5 * sqrt(max(tr - 2.0 * det2, 0.0));
      float v2 = 0.5 * sqrt(max(tr + 2.0 * det2, 0.0));
      float e_max = (u + v2) * (u + v2);
      float e_min = (u - v2) * (u - v2);

      float3 V1n, V2n;
      if (d11 > d22)
      {
        V1n = d12 * V1_ + (e_max - d11) * V2_;
        V2n = d12 * V1_ + (e_min - d11) * V2_;
      }
      else
      {
        V1n = d12 * V2_ + (e_max - d22) * V1_;
        V2n = d12 * V2_ + (e_min - d22) * V1_;
      }

      a = 1.0 / max(e_max, 1e-7);
      b = 1.0 / max(e_min, 1e-7);
      V1_ = LTC_SafeNormalize(V1n);
      V2_ = LTC_SafeNormalize(V2n);

      if (dot(V1_, V1_) < 0.5 || dot(V2_, V2_) < 0.5)
      {
        if (i == 0) specular = 0;
        else diffuse = 0;
        continue;
      }
    }
    else
    {
      a = 1.0 / max(d11, 1e-7);
      b = 1.0 / max(d22, 1e-7);
      V1_ *= sqrt(a);
      V2_ *= sqrt(b);
    }

    float3 V3_ = cross(V1_, V2_);
    if (dot(C_, V3_) < 0.0)
      V3_ *= -1.0;

    float L = max(dot(V3_, C_), LTC_NAN_EPSILON);
    float x0 = dot(V1_, C_) / L;
    float y0 = dot(V2_, C_) / L;

    a *= L * L;
    b *= L * L;

    float c0 = a * b;
    float c1 = a * b * (1.0 + x0 * x0 + y0 * y0) - a - b;
    float c2 = 1.0 - a * (1.0 + x0 * x0) - b * (1.0 + y0 * y0);
    float c3 = 1.0;

    float3 roots = SolveCubic(float4(c0, c1, c2, c3));

    float e2 = roots.y;

    float3 avgDir = float3(a * x0 / max(a - e2, 1e-7), b * y0 / max(b - e2, 1e-7), 1.0);
    float3x3 rotMat = float3x3(V1_, V2_, V3_);
    avgDir = mul(avgDir, rotMat);
    avgDir = LTC_SafeNormalize(avgDir);

    float L1 = sqrt(max(-e2 / roots.z, 0.0));
    float L2 = sqrt(max(-e2 / roots.x, 0.0));
    float formFactor = L1 * L2 * rsqrt(max((1.0 + L1 * L1) * (1.0 + L2 * L2), 1e-7));

    float2 sphereUV = float2(avgDir.z * 0.5 + 0.5, formFactor);
    sphereUV = sphereUV * LTC_LUT_SCALE + LTC_LUT_BIAS;
    float scale = LtcMagTexture.SampleLevel(LtcSampler, sphereUV, 0).w;

    float result = formFactor * scale;

    if (i == 0) specular = result;
    else diffuse = result;
  }

  float3 F = matData.specularColor * t2.x + (1.0 - matData.specularColor) * t2.y;

  return InitializeLight(matData.diffuseColor * LTC_Sanitize(diffuse), F * LTC_Sanitize(specular));
}

// Cylinder lights are functional but do not look up to quality so not implemented for now.

//
// // === Cylinder Light (LTC analytic line integral) ===
// // Based on "Real-Time Line- and Disk-Light Shading with Linearly Transformed Cosines"
//
// float Fpo(float d, float l)
// {
//   float dSq = d * d;
//   return l / (d * (dSq + l * l)) + atan2(l, d) / dSq;
// }
//
// float Fwt(float d, float l)
// {
//   return l * l / (d * (d * d + l * l));
// }
//
// float I_diffuse_line(float3 p1, float3 p2)
// {
//   float3 diff = p2 - p1;
//   float diffLenSq = dot(diff, diff);
//   if (diffLenSq < LTC_NAN_EPSILON) return 0.0;
//   float3 wt = diff * rsqrt(diffLenSq);
//
//   if (p1.z <= 0.0 && p2.z <= 0.0) return 0.0;
//
//   float zDenom1 = p2.z - p1.z;
//   if (p1.z < 0.0 && abs(zDenom1) > LTC_NAN_EPSILON)
//     p1 = (p1 * p2.z - p2 * p1.z) / zDenom1;
//
//   float zDenom2 = p1.z - p2.z;
//   if (p2.z < 0.0 && abs(zDenom2) > LTC_NAN_EPSILON)
//     p2 = (p2 * p1.z - p1 * p2.z) / zDenom2;
//
//   float l1 = dot(p1, wt);
//   float l2 = dot(p2, wt);
//
//   float3 po = p1 - l1 * wt;
//
//   float d = max(length(po), 1e-5);
//
//   float I = (Fpo(d, l2) - Fpo(d, l1)) * po.z +
//             (Fwt(d, l2) - Fwt(d, l1)) * wt.z;
//   return I / PI;
// }
//
// float I_ltc_line(float3x3 Minv, float3 p1, float3 p2)
// {
//   float3 p1o = mul(Minv, p1);
//   float3 p2o = mul(Minv, p2);
//   float I = I_diffuse_line(p1o, p2o);
//
//   float3 crossP = cross(p1, p2);
//   float crossLenSq = dot(crossP, crossP);
//   if (crossLenSq < LTC_NAN_EPSILON) return 0.0;
//   float3 ortho = crossP * rsqrt(crossLenSq);
//
//   float3x3 MinvT;
//   MinvT[0] = float3(Minv[0][0], Minv[1][0], Minv[2][0]);
//   MinvT[1] = float3(Minv[0][1], Minv[1][1], Minv[2][1]);
//   MinvT[2] = float3(Minv[0][2], Minv[1][2], Minv[2][2]);
//   float3 MinvTOrtho = mul(MinvT, ortho);
//   float w = 1.0 / max(length(MinvTOrtho), 1e-7);
//
//   return w * I;
// }
//
// float I_ltc_disks(float3x3 Minv, float3 p1, float3 p2, float R)
// {
//   float A = PI * R * R;
//   float3 diff = p2 - p1;
//   float diffLenSq = dot(diff, diff);
//   if (diffLenSq < LTC_NAN_EPSILON) return 0.0;
//   float3 wt = diff * rsqrt(diffLenSq);
//
//   float3 p1o = mul(Minv, p1);
//   float3 p2o = mul(Minv, p2);
//
//   float detMinv = determinant(Minv);
//
//   float lo1 = max(length(p1o), 1e-5);
//   float3 wp1 = p1o / lo1;
//   float D1 = 1.0 / PI * max(0.0, wp1.z) * abs(detMinv) / (lo1 * lo1 * lo1);
//
//   float lo2 = max(length(p2o), 1e-5);
//   float3 wp2 = p2o / lo2;
//   float D2 = 1.0 / PI * max(0.0, wp2.z) * abs(detMinv) / (lo2 * lo2 * lo2);
//
//   float p1LenSq = max(dot(p1, p1), 1e-7);
//   float p2LenSq = max(dot(p2, p2), 1e-7);
//   float Idisks = A * (
//     D1 * max(0.0, dot(+wt, p1 * rsqrt(p1LenSq))) / p1LenSq +
//     D2 * max(0.0, dot(-wt, p2 * rsqrt(p2LenSq))) / p2LenSq);
//
//   return Idisks;
// }
//
// AccumulatedLight CylinderLightShading(ezPerLightData lightData, ezMaterialData matData, float3 V)
// {
//   float3 N = matData.worldNormal;
//   float3 P = matData.worldPosition;
//
//   float NdotV = saturate(dot(N, V));
//
//   float2 uv = float2(matData.roughness, sqrt(1.0 - NdotV));
//   uv = uv * LTC_LUT_SCALE + LTC_LUT_BIAS;
//
//   float4 t1 = LtcMatTexture.SampleLevel(LtcSampler, uv, 0);
//   float4 t2 = LtcMagTexture.SampleLevel(LtcSampler, uv, 0);
//
//   float3x3 Minv = float3x3(
//     float3(t1.x, 0, t1.z),
//     float3(  0,  1,   0 ),
//     float3(t1.y, 0, t1.w)
//   );
//
//   float3 lightPos = lightData.position;
//   float3 lightForward = GetLightDirection(lightData);
//   float3 axisDir = GetCylinderAxisDirection(lightData);
//   float2 radiusAndHalfLen = GetCylinderRadiusAndHalfLength(lightData);
//   float cylRadius = radiusAndHalfLen.x;
//   float halfLength = radiusAndHalfLen.y;
//   bool endCaps = lightData.Unused3 > 0.5;
//
//   float3 endpoint1 = lightPos - axisDir * halfLength;
//   float3 endpoint2 = lightPos + axisDir * halfLength;
//
//   float3 T1 = LTC_BuildTangent(V, N);
//   float3 T2 = cross(N, T1);
//   float3x3 B = float3x3(T1, T2, N);
//
//   float3 p1 = mul(B, endpoint1 - P);
//   float3 p2 = mul(B, endpoint2 - P);
//
//   // Specular
//   float3x3 Mspec = mul(Minv, B);
//   float specLine = cylRadius * I_ltc_line(Minv, p1, p2);
//   float specDisks = endCaps ? I_ltc_disks(Minv, p1, p2, cylRadius) : 0.0;
//   float specular = min(1.0, specLine + specDisks);
//
//   // Diffuse (identity Minv)
//   float3x3 Midentity = float3x3(
//     float3(1, 0, 0),
//     float3(0, 1, 0),
//     float3(0, 0, 1)
//   );
//   float diffLine = cylRadius * I_ltc_line(Midentity, p1, p2);
//   float diffDisks = endCaps ? I_ltc_disks(Midentity, p1, p2, cylRadius) : 0.0;
//   float diffuse = diffLine + diffDisks;
//
//   float3 F = matData.specularColor * t2.x + (1.0 - matData.specularColor) * t2.y;
//
//   float sanitizedDiff = LTC_Sanitize(diffuse / (2.0 * PI));
//   float sanitizedSpec = LTC_Sanitize(specular / (2.0 * PI));
//
//   return InitializeLight(matData.diffuseColor * sanitizedDiff, F * sanitizedSpec);
// }
