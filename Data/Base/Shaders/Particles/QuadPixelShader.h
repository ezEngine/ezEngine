// clang-format off
#include <Shaders/Common/StandardMacros.h>
#include <Shaders/Particles/ParticleCommonPS.h>

Texture2D ParticleTexture BIND_GROUP(BG_MATERIAL);
SamplerState ParticleTexture_AutoSampler BIND_GROUP(BG_MATERIAL);

#if PARTICLE_RENDER_MODE == PARTICLE_RENDER_MODE_DISTORTION

  Texture2D ParticleDistortionTexture BIND_GROUP(BG_MATERIAL);
  SamplerState ParticleDistortionTexture_AutoSampler BIND_GROUP(BG_MATERIAL);

#endif

float4 main(PS_IN Input) : SV_Target
{
#if CAMERA_MODE == CAMERA_MODE_STEREO
  s_ActiveCameraEyeIndex = Input.RenderTargetArrayIndex;
#endif

  float4 texCol = ParticleTexture.Sample(ParticleTexture_AutoSampler, Input.TexCoord0.xy);

  float proximityFadeOut = 1.0;

  #if CAMERA_MODE != CAMERA_MODE_ORTHO
    proximityFadeOut = CalcProximityFadeOut(Input.Position);
  #endif

  float opacity = Input.Color0.a * texCol.a * proximityFadeOut;

  #if PARTICLE_RENDER_MODE == PARTICLE_RENDER_MODE_DISTORTION
    if (opacity <= 1.0 / 255.0)
    {
      discard;
    }

    float4 texDistort = ParticleDistortionTexture.Sample(ParticleDistortionTexture_AutoSampler, Input.TexCoord0.xy);
    float3 sceneColor = SampleSceneColor(Input.Position.xy + (texDistort - 0.5) * float2(DistortionStrength, DistortionStrength));

    return float4(sceneColor.rgb * Input.Color0.rgb, opacity);

  #else

    float3 finalColor = texCol.rgb * Input.Color0.rgb;

    if (opacity <= 1.0 / 255.0)
    {
      discard;
      return float4(1, 0, 1, 1);
    }

    #if PARTICLE_RENDER_MODE == PARTICLE_RENDER_MODE_ADDITIVE
      opacity *= Input.FogAmount;
    #elif PARTICLE_RENDER_MODE == PARTICLE_RENDER_MODE_OPAQUE
      opacity = 1.0f;
    #else
      finalColor = ApplyFog(finalColor, Input.FogAmount);
    #endif

    #if defined(RENDER_PASS) && RENDER_PASS == RENDER_PASS_EDITOR
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
    #else
      return float4(finalColor, opacity);
    #endif

  #endif
}
// clang-format on
