#define CUSTOM_INTERPOLATOR float FogAmount : FOG;

#include <Shaders/Particles/ParticleCommonPS.h>

Texture2D ParticleTexture;
SamplerState ParticleTexture_AutoSampler;

#if PARTICLE_RENDER_MODE == PARTICLE_RENDER_MODE_DISTORTION

  Texture2D ParticleDistortionTexture;
  SamplerState ParticleDistortionTexture_AutoSampler;

#endif

float4 main(PS_IN Input) : SV_Target
{
  float4 texCol = ParticleTexture.Sample(ParticleTexture_AutoSampler, Input.TexCoord0.xy);

  float proximityFadeOut = 1.0;

  #if CAMERA_MODE != CAMERA_MODE_ORTHO
    proximityFadeOut = CalcProximityFadeOut(Input.Position);
  #endif

  float opacity = Input.Color0.a * texCol.a * proximityFadeOut;


  #if PARTICLE_RENDER_MODE == PARTICLE_RENDER_MODE_DISTORTION
    if (opacity == 0.0)
    {
      discard;
    }

    float4 texDistort = ParticleDistortionTexture.Sample(ParticleDistortionTexture_AutoSampler, Input.TexCoord0.xy);
    float3 sceneColor = SampleSceneColor(Input.Position.xy + (texDistort - 0.5) * float2(DistortionStrength, DistortionStrength));

    return float4(sceneColor.rgb * Input.Color0.rgb, opacity);

  #else

    float3 finalColor = texCol.rgb * Input.Color0.rgb;

    #if PARTICLE_RENDER_MODE == PARTICLE_RENDER_MODE_ADDITIVE
      opacity *= Input.FogAmount;
    #else
      finalColor = ApplyFog(finalColor, Input.FogAmount);
    #endif

    if (opacity == 0.0)
    {
      discard;
    }

    return float4(finalColor, opacity);

  #endif
}