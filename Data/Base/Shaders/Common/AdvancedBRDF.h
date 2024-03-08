#pragma once

#include <Shaders/Materials/MaterialData.h>

float3 ComputeDiffuseEnergy(float3 fresnel, float metalness)
{
  // Diffuse scattering happens due to light being refracted multiple times by a dielectric medium.
  // Metals on the other hand either reflect or absorb energy, so diffuse contribution is always zero.
  // To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness.

  float3 kS = fresnel;         // The energy of light that gets reflected - Equal to Fresnel
  float3 kD = 1.0f - kS; // Remaining energy, light that gets refracted
  kD *= 1.0f - metalness; // Multiply kD by the inverse metalness such that only non-metals have diffuse lighting

  return kD;
}

/*------------------------------------------------------------------------------
    Fresnel
------------------------------------------------------------------------------*/

float3 F_Schlick(const float3 f0, float f90, float NdotH)
{
  // Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
  return f0 + (f90 - f0) * pow(1.0 - NdotH, 5.0);
}

float3 F_Schlick(const float3 f0, float NdotH)
{
  float f = pow(1.0 - NdotH, 5.0);
  return f + f0 * (1.0 - f);
}

float3 F_SchlickRoughness(float3 f0, float NdotH, float roughness)
{
  float3 a = 1.0 - roughness;
  return f0 + (max(a, f0) - f0) * pow(max(1.0 - NdotH, 0.0), 5.0);
}

/*------------------------------------------------------------------------------
    Visibility
------------------------------------------------------------------------------*/

// Smith term for GGX
// [Smith 1967, "Geometrical shadowing of a random rough surface"]
inline float V_Smith(float a2, float NoV, float NoL)
{
  float Vis_SmithV = NoV + sqrt(NoV * (NoV - NoV * a2) + a2);
  float Vis_SmithL = NoL + sqrt(NoL * (NoL - NoL * a2) + a2);
  return rcp(Vis_SmithV * Vis_SmithL);
}

// Appoximation of joint Smith term for GGX
// [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
inline float V_SmithJointApprox(float a, float NoV, float NoL)
{
  float Vis_SmithV = NoL * (NoV * (1 - a) + a);
  float Vis_SmithL = NoV * (NoL * (1 - a) + a);
  return saturate_16(0.5 * rcp(Vis_SmithV + Vis_SmithL));
}

float V_GGX_anisotropic_2cos(float cos_theta_m, float alpha_x, float alpha_y, float cos_phi, float sin_phi)
{
  float cos2  = cos_theta_m * cos_theta_m;
  float sin2  = (1.0 - cos2);
  float s_x   = alpha_x * cos_phi;
  float s_y   = alpha_y * sin_phi;
  return 1.0 / max(cos_theta_m + sqrt(cos2 + (s_x * s_x + s_y * s_y) * sin2), 0.001);
}

// [Kelemen 2001, "A microfacet based coupled specular-matte brdf model with importance sampling"]
float V_Kelemen(float NoH)
{
  // constant to prevent NaN
  return rcp(4 * NoH * NoH + 1e-5);
}

// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
float V_Neubelt(float NoV, float NoL)
{
  return saturate_16(1.0 / (4.0 * (NoL + NoV - NoL * NoV)));
}

/*------------------------------------------------------------------------------
    Distribution
------------------------------------------------------------------------------*/

// GGX / Trowbridge-Reitz
// [Walter et al. 2007, "Microfacet models for refraction through rough surfaces"]
float D_GGX(float a2, float NoH)
{
  float d = (NoH * a2 - NoH) * NoH + 1; // 2 mad
  return a2 / ( d * d); // 4 mul, 1 rcp
}

float D_GGX_Anisotropic(float cos_theta_m, float alpha_x, float alpha_y, float cos_phi, float sin_phi)
{
  float cos2  = cos_theta_m * cos_theta_m;
  float sin2  = (1.0 - cos2);
  float r_x   = cos_phi / alpha_x;
  float r_y   = sin_phi / alpha_y;
  float d     = cos2 + sin2 * (r_x * r_x + r_y * r_y);
  return saturate_16(1.0 / (PI * alpha_x * alpha_y * d * d));
}

float D_Charlie(float roughness, float NoH)
{
  // Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF"
  float invAlpha  = 1.0 / roughness;
  float cos2h     = NoH * NoH;
  float sin2h     = max(1.0 - cos2h, 0.0078125); // 2^(-14/2), so sin2h^2 > 0 in fp16
  return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) / (2.0 * PI);
}

/*------------------------------------------------------------------------------
    Diffuse
------------------------------------------------------------------------------*/

float3 Diffuse_Lambert(ezMaterialData matData, float NoV, float NoL, float VoH, float NdotH)
{
  return matData.diffuseColor;
}

// [Burley 2012, "Physically-Based Shading at Disney"]
float3 Diffuse_Burley(ezMaterialData matData, float NoV, float NoL, float VoH, float NdotH)
{
  float FD90 = 0.5 + 2 * VoH * VoH * matData.roughness;
  float FdV = 1 + (FD90 - 1) * pow(1 - NoV, 5);
  float FdL = 1 + (FD90 - 1) * pow(1 - NoL, 5);
  return matData.diffuseColor * (FdV * FdL );
}

// Diffuse - [Gotanda 2012, "Beyond a Simple Physically Based Blinn-Phong Model in Real-Time"]
float3 Diffuse_OrenNayar(ezMaterialData matData, float NoV, float NoL, float VoH, float NdotH)
{
  float a     = matData.roughness;
  float s     = a;                    // ( 1.29 + 0.5 * a );
  float s2    = s * s;
  float VoL   = 2 * VoH * VoH - 1;    // double angle identity
  float Cosri = VoL - NoV * NoL;
  float C1    = 1 - 0.5 * s2 / (s2 + 0.33);
  float C2    = 0.45 * s2 / (s2 + 0.09) * Cosri * ( Cosri >= 0 ? rcp( max( NoL, NoV + 0.0001f ) ) : 1 );
  return matData.diffuseColor * ( C1 + C2 ) * ( 1 + matData.roughness * 0.5 );
}

float3 Diffuse_Chan(ezMaterialData matData, float NoV, float NoL, float VoH, float NoH)
{
  NoV = saturate(NoV);
  NoL = saturate(NoL);
  VoH = saturate(VoH);
  NoH = saturate(NoH);

  float g = saturate( (1.0 / 18.0) * log2( 2 * rcp(matData.roughness) - 1 ) );

  float F0 = VoH + pow( 1 - VoH, 5.0);
  float FdV = 1 - 0.75 * pow( 1 - NoV, 5.0);
  float FdL = 1 - 0.75 * pow( 1 - NoL, 5.0);

  // Rough (F0) to smooth (FdV * FdL) response interpolation
  float Fd = lerp( F0, FdV * FdL, saturate( 2.2 * g - 0.5 ) );

  // Retro reflectivity contribution.
  float Fb = ( (34.5 * g - 59 ) * g + 24.5 ) * VoH * exp2( -max( 73.2 * g - 21.2, 8.9 ) * sqrt( NoH ) );

  return matData.diffuseColor * (( Fd + Fb ) );
}


float3 BRDF_Diffuse(ezMaterialData matData, float NoV, float NoL, float VoH, float NoH)
{
#if BRDF_DIFFUSE == BRDF_DIFFUSE_LAMBERT
  return Diffuse_Lambert(matData, NoV, NoL, VoH, NoH);
#elif BRDF_DIFFUSE == BRDF_DIFFUSE_BURLEY
  return Diffuse_Burley(matData, NoV, NoL, VoH, NoH);
#elif BRDF_DIFFUSE == BRDF_DIFFUSE_OREN_NAYAR
  return Diffuse_OrenNayar(matData, NoV, NoL, VoH, NoH);
#else
  return Diffuse_Chan(matData, NoV, NoL, VoH, NoH);
#endif
}

float3 BRDF_Specular_Isotropic(
  ezMaterialData matData,
  float NoV,
  float NoL,
  float NoH,
  float VoH,
  float LoH
)
{
  float a  = matData.roughness;
  float a2 = a * a;

  float  V = V_Smith(a2, NoV, NoL);
  float  D = D_GGX(a2, NoH);
  float3 F = F_Schlick(matData.specularColor, VoH);

  return (D * V ) * F;
}

#if defined(USE_MATERIAL_SPECULAR_ANISOTROPIC)
float3 BRDF_Specular_Anisotropic(ezMaterialData matData, float3 h, float NoV, float NoL, float NoH, float LoH)
{
  // Construct TBN from the normal
  float3 t, b;
  FindBestAxisVectors(matData.worldNormal, t, b);
  float3x3 TBN = float3x3(t, b, matData.worldNormal);

  // Rotate tangent and bitagent
  float rotation   = max(matData.anisotropicRotation * PI2, FLT_MIN); // convert material property to a full rotation
  float2 direction = float2(cos(rotation), sin(rotation));             // convert rotation to direction
  t                = normalize(mul(float3(direction, 0.0f), TBN).xyz); // compute direction derived tangent
  b                = normalize(cross(matData.worldNormal, t));              // re-compute bitangent

  float alpha_ggx = matData.roughness;
  float aspect    = sqrt(1.0 - matData.anisotropic * 0.9);
  float ax        = alpha_ggx / aspect;
  float ay        = alpha_ggx * aspect;
  float XdotH     = dot(t, h);
  float YdotH     = dot(b, h);

  // specular anisotropic BRDF
  float D   = D_GGX_Anisotropic(NoH, ax, ay, XdotH, YdotH);
  float V   = V_GGX_anisotropic_2cos(NoV, ax, ay, XdotH, YdotH) * V_GGX_anisotropic_2cos(NoV, ax, ay, XdotH, YdotH);
  float f90 = saturate(dot(matData.specularColor, 50.0 * GetLuminance(matData.specularColor)));
  float3 F  = F_Schlick(matData.specularColor, f90, LoH);

  return (D * V * NoL) * F;
}
#endif

#if defined(USE_MATERIAL_SPECULAR_CLEARCOAT)
float3 BRDF_Specular_Clearcoat(ezMaterialData matData, float NdotH, float VdotH)
{
  // float a2 = pow4(matData.roughness);
  float a2 = matData.clearcoatRoughness * matData.clearcoatRoughness;
  float a4 = a2 * a2;

  float D  = D_GGX(a4, NdotH);
  float V  = V_Kelemen(VdotH);
  float3 F = F_Schlick(0.04, 1.0, VdotH) * matData.clearcoat;

  return D * V * F;
}
#endif

#if defined(USE_MATERIAL_SPECULAR_SHEEN)
float3 BRDF_Specular_Sheen(ezMaterialData matData, float NdotV, float NdotL, float NdotH)
{
  // Mix between white and using base color for sheen reflection
  float tint = matData.sheenTintFactor * matData.sheenTintFactor;
  float3 f0  = lerp(1.0f, matData.specularColor, tint);

  float D  = D_Charlie(matData.roughness, NdotH);
  float V  = V_Neubelt(NdotV, NdotL);
  float3 F = f0 * matData.sheen;

  return D * V * F;
}
#endif

AccumulatedLight DefaultShadingNew(ezMaterialData matData, float3 L, float3 V)
{
  float3 N = matData.worldNormal;
  float3 H = normalize(V + L);

  float NdotL = saturate( dot(N, L) );
  float NdotV = max( dot(N, V), 1e-5f );
  float NdotH = saturate( dot(N, H) );
  float VdotH = saturate( dot(V, H) );
  float LdotH = saturate( dot(L, H) );

  float3 specular = 0.0f;
  float3 diffuse  = 0.0f;


  // Specular
 #if defined(USE_MATERIAL_SPECULAR_ANISOTROPIC)
   if (matData.anisotropic != 0.0f)
   {
     specular += BRDF_Specular_Anisotropic(matData, H, NdotV, NdotL, NdotH, LdotH) * NdotL;
   }
   else
 #endif
 {
    specular += BRDF_Specular_Isotropic(matData, NdotV, NdotL, NdotH, VdotH, LdotH) * NdotL;
 }

 #if defined(USE_MATERIAL_SPECULAR_CLEARCOAT)
   matData.clearcoat = 1.0;
   // Specular clearcoat
   if (matData.clearcoat != 0.0f)
   {
     float cNoH = saturate(dot(matData.clearcoatNormal, H));
     float cNoL = saturate(dot(matData.clearcoatNormal, L));
     specular += BRDF_Specular_Clearcoat(matData, cNoH, VdotH) * cNoL;
  }
 #endif

#if defined(USE_MATERIAL_SPECULAR_SHEEN)
  // Specular sheen
  if (matData.sheen != 0.0f)
  {
    specular += BRDF_Specular_Sheen(matData, NdotV, NdotL, NdotH) * NdotL;
  }
#endif

  diffuse  = BRDF_Diffuse(matData, NdotV, NdotL, VdotH, NdotH) * NdotL;


  return InitializeLight(diffuse, specular);
}