#include <Shaders/Common/StandardMacros.h>
#include <Shaders/Particles/ParticleCommonPS.h>

void FillCustomGlobals();
float GetParticleOpacity();
float3 GetParticleColor();

#if PARTICLE_RENDER_MODE == PARTICLE_RENDER_MODE_NONE
struct TMP_PS_IN
{
  float4 Position : SV_Position;
  float2 TexCoord0 : TEXCOORD0;
  float4 Color0 : COLOR0;
  float FogAmount : FOG;
  float Life : TEXCOORD1;
  float Variation : TEXCOORD2;
};

struct PS_GLOBALS
{
  TMP_PS_IN Input;
};
static PS_GLOBALS G;

float4 main(TMP_PS_IN Input)
  : SV_Target
{
  G.Input = Input;
  FillCustomGlobals();
  return float4(GetParticleColor(), 1.0);
}

#else

struct PS_GLOBALS
{
  PS_IN Input;
};
static PS_GLOBALS G;

float4 main(PS_IN Input)
  : SV_Target
{
  G.Input = Input;
  FillCustomGlobals();

#  if CAMERA_MODE == CAMERA_MODE_STEREO
  s_ActiveCameraEyeIndex = Input.RenderTargetArrayIndex;
#  endif

  float proximityFadeOut = 1.0;

#  if CAMERA_MODE != CAMERA_MODE_ORTHO
  proximityFadeOut = CalcProximityFadeOut(Input.Position);
#  endif

  float opacity = proximityFadeOut * GetParticleOpacity();
  if (opacity <= 1.0 / 255.0)
  {
    discard;
  }

  float3 finalColor = GetParticleColor();

#  if PARTICLE_RENDER_MODE == PARTICLE_RENDER_MODE_OPAQUE
  opacity = 1.0;
#  endif

// fog behavior
#  if PARTICLE_RENDER_MODE == PARTICLE_RENDER_MODE_ADDITIVE
  // for additive blending, color shouldn't be adjusted, just fade out
  opacity *= Input.FogAmount;
#  else
  finalColor = ApplyFog(finalColor, Input.FogAmount);
#  endif

#  if defined(RENDER_PASS) && RENDER_PASS == RENDER_PASS_EDITOR
  if (RenderPass == EDITOR_RENDER_PASS_DIFFUSE_LIT_ONLY)
  {
    return float4(SrgbToLinear(Input.Color0.rgb * Exposure), 1);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_TEXCOORDS_UV0)
  {
    return float4(SrgbToLinear(float3(frac(Input.TexCoord0.xy), 0)), 1);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_DIFFUSE_COLOR || RenderPass == EDITOR_RENDER_PASS_DIFFUSE_COLOR_RANGE)
  {
    return float4(finalColor, 1);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_PIXEL_NORMALS || RenderPass == EDITOR_RENDER_PASS_VERTEX_NORMALS)
  {
    return float4(SrgbToLinear(normalize(Input.Color0.rgb) * 0.5 + 0.5), 1);
  }
  else if (RenderPass == EDITOR_RENDER_PASS_OCCLUSION)
  {
    return 1;
  }
  else if (RenderPass == EDITOR_RENDER_PASS_DEPTH)
  {
    float depth = Input.Position.w * ClipPlanes.z;
    return float4(SrgbToLinear(depth), 1);
  }
  else
  {
    return float4(0, 0, 0, 1);
  }
#  else
  return float4(finalColor, opacity);
#  endif
}

#endif
