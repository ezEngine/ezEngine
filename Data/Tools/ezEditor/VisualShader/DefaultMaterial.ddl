Node %MaterialOutput
{
  string %Category { "Output" }
  unsigned_int8 %Color { 127, 0, 110 }
  string %NodeType { "Main" }
  string %CodePermutations { "
BLEND_MODE
RENDER_PASS
SHADING_MODE
TWO_SIDED
FLIP_WINDING
FORWARD_PASS_WRITE_DEPTH
MSAA
SHADING_QUALITY
CAMERA_MODE" }

  string %CheckPermutations
  {"
BLEND_MODE=BLEND_MODE_OPAQUE
RENDER_PASS=RENDER_PASS_FORWARD
SHADING_MODE=SHADING_MODE_LIT
TWO_SIDED=FALSE
FLIP_WINDING=FALSE
FORWARD_PASS_WRITE_DEPTH=TRUE
MSAA=FALSE
SHADING_QUALITY=SHADING_QUALITY_NORMAL
CAMERA_MODE=CAMERA_MODE_PERSPECTIVE
"}

  string %CodeRenderStates { "#include <Shaders/Materials/MaterialState.h>" }
  string %CodeVertexShader { "
#define USE_NORMAL
#define USE_TANGENT
#define USE_TEXCOORD0

#if RENDER_PASS == RENDER_PASS_EDITOR
  #define USE_DEBUG_INTERPOLATOR
#endif

#if INPUT_PIN_9_CONNECTED
  #define USE_OBJECT_POSITION_OFFSET
#endif

#if INPUT_PIN_10_CONNECTED
  #define USE_WORLD_POSITION_OFFSET
#endif

#include <Shaders/Materials/DefaultMaterialCB.h>
#include <Shaders/Materials/MaterialVertexShader.h>
#include <Shaders/Common/VisualShaderUtil.h>

VS_OUT main(VS_IN Input)
{
  return FillVertexData(Input);
}

#if defined(USE_OBJECT_POSITION_OFFSET)
float3 GetObjectPositionOffset(VS_IN Input, ezPerInstanceData data)
{
  return ToFloat3($in9);
}
#endif

#if defined(USE_WORLD_POSITION_OFFSET)
float3 GetWorldPositionOffset(VS_IN Input, ezPerInstanceData data, float3 worldPosition)
{
  return ToFloat3($in10);
}
#endif

" }

  string %CodeGeometryShader { "

#define USE_NORMAL
#define USE_TANGENT
#define USE_TEXCOORD0

#include <Shaders/Materials/MaterialStereoGeometryShader.h>

" }

  string %CodeMaterialParams { "
Permutation BLEND_MODE;
Permutation SHADING_MODE;
Permutation TWO_SIDED;
float MaskThreshold @Default($prop0);
" }

  string %CodePixelDefines { "
#define USE_NORMAL
#define USE_TANGENT
#define USE_TEXCOORD0
#define USE_SIMPLE_MATERIAL_MODEL
#define USE_MATERIAL_EMISSIVE
#define USE_MATERIAL_OCCLUSION
#define USE_TWO_SIDED_LIGHTING
#define USE_DECALS

#if RENDER_PASS == RENDER_PASS_EDITOR
  #define USE_DEBUG_INTERPOLATOR
#endif

#if $prop1
  #define USE_FOG
#endif

#if INPUT_PIN_8_CONNECTED
  #define USE_MATERIAL_REFRACTION
#endif
" }

  string %CodePixelIncludes { "
#include <Shaders/Materials/DefaultMaterialCB.h>
#include <Shaders/Materials/MaterialPixelShader.h>
#include <Shaders/Common/VisualShaderUtil.h>
" }

  string %CodePixelSamplers { "" }
  string %CodePixelConstants { "" }
  string %CodePixelBody { "

float3 GetBaseColor(PS_IN Input)
{
  return ToColor3($in0);
}

float3 GetNormal(PS_IN Input)
{
  return TangentToWorldSpace(ToFloat3($in1), Input);
}

float GetMetallic(PS_IN Input)
{
  return saturate(ToFloat1($in2));
}

float GetReflectance(PS_IN Input)
{
  return saturate(ToFloat1($in3));
}

float GetRoughness(PS_IN Input)
{
  return saturate(ToFloat1($in4));
}

float GetOpacity(PS_IN Input)
{
  #if BLEND_MODE == BLEND_MODE_MASKED
    return saturate(ToFloat1($in5)) - MaskThreshold;
  #else
    return saturate(ToFloat1($in5));
  #endif
}

float3 GetEmissiveColor(PS_IN Input)
{
  return ToColor3($in6);
}

float GetOcclusion(PS_IN Input)
{
  return saturate(ToFloat1($in7));
}

#if defined USE_MATERIAL_REFRACTION
float4 GetRefractionColor(PS_IN Input)
{
  return ToColor4($in8);
}
#endif

" }

  Property %MaskThreshold
  {
    string %Type { "float" }
    string %DefaultValue { "0.25" }
  }
  
  Property %ApplyFog
  {
    string %Type { "bool" }
    string %DefaultValue { "true" }
  }

  // Pin 0
  InputPin %BaseColor
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 255, 255, 255 }
    bool %Expose { true }
    string %DefaultValue { "1, 1, 1" }
  }

  // Pin 1
  InputPin %Normal
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 128, 128, 255 }
    string %DefaultValue { "float3(0, 0, 1)", }
    string %Tooltip { "Surface normal in tangent space." }
  }

  // Pin 2
  InputPin %Metallic
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 128, 128 }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  // Pin 3
  InputPin %Reflectance
  {
    string %Type { "float" }
    unsigned_int8 %Color { 210, 255, 100 }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }

  // Pin 4
  InputPin %Roughness
  {
    string %Type { "float" }
    unsigned_int8 %Color { 150, 64, 64 }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }

  // Pin 5
  InputPin %Opacity
  {
    string %Type { "float" }
    unsigned_int8 %Color { 255, 0, 110 }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }

  // Pin 6
  InputPin %Emissive
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 255, 106, 0 }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  // Pin 7
  InputPin %Occlusion
  {
    string %Type { "float" }
    unsigned_int8 %Color { 127, 115, 63 }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  
  // Pin 8
  InputPin %RefractionColor
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 255, 106, 0 }
    bool %Expose { true }
    string %DefaultValue { "0, 0, 0, 1" }
  }

  // Pin 9
  InputPin %LocalPosOffset
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 75, 145, 112 }
    string %DefaultValue { "0" }
  }

  // Pin 10
  InputPin %GlobalPosOffset
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 226, 96, 93 }
    string %DefaultValue { "0" }
  }
}
