Node %QuadParticleOutput
{
  string %Category { "Output" }
  string %Color { "Grape" }
  string %NodeType { "Main" }
  string %Docs { "Output node for quad particles." }

  string %CodePermutations { "
RENDER_PASS
PARTICLE_RENDER_MODE
PARTICLE_QUAD_MODE
PARTICLE_LIGHTING_MODE
SHADING_QUALITY
CAMERA_MODE
" }

  string %CheckPermutations {"
RENDER_PASS=RENDER_PASS_FORWARD
CAMERA_MODE=CAMERA_MODE_PERSPECTIVE
SHADING_QUALITY=SHADING_QUALITY_NORMAL
PARTICLE_RENDER_MODE=PARTICLE_RENDER_MODE_ADDITIVE
PARTICLE_QUAD_MODE=PARTICLE_QUAD_MODE_BILLBOARD
PARTICLE_LIGHTING_MODE=PARTICLE_LIGHTING_MODE_FULLBRIGHT
"}

  string %CodeRenderStates { "#include <Shaders/Particles/ParticleRenderState.h>" }

  string %CodeShaderShared { "
#define CUSTOM_INTERPOLATOR float FogAmount : FOG; float Life : TEXCOORD1; float Variation : TEXCOORD2;
#ifndef USE_TEXCOORD0
#define USE_TEXCOORD0
#endif
#ifndef USE_COLOR0
#define USE_COLOR0
#endif
" }

  string %CodeVertexShader { "
#include <Shaders/Particles/QuadParticleVertexShader.h>
" }

  string %CodeMaterialParams { "" }

  string %CodePixelIncludes { "
#include <Shaders/Common/VisualShaderUtil.h>
#include <Shaders/Particles/ParticlePixelShader.h>
" }

  string %CodePixelSamplers { "" }
  string %CodePixelConstants { "" }
  string %CodeMaterialConstants { "

  // Insert custom Visual Shader parameters here
  VSE_CONSTANTS
" }

  string %CodePixelBody { "

void FillCustomGlobals()
{
}
  
float3 GetParticleColor()
{
  return ToColor3($in0);
}

float GetParticleOpacity()
{
  return saturate(ToFloat1($in1));
}

" }

  // Pin 0
  InputPin %Color
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    bool %Expose { true }
    string %DefaultValue { "1, 1, 1" }
  }

  // Pin 1
  InputPin %Opacity
  {
    string %Type { "float" }
    string %Color { "Red" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
}
