Node %MaterialOutput
{
  string %Category { "Output" }
  string %Color { "Grape" }
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
CAMERA_MODE
VERTEX_SKINNING
VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX" }

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
VERTEX_SHADER_RENDER_TARGET_ARRAY_INDEX=TRUE
VERTEX_SKINNING=FALSE
"}

  string %CodeRenderStates { "#include <Shaders/Materials/MaterialState.h>" }
  string %CodeVertexShader { "

#if VERTEX_SKINNING
  #define USE_SKINNING
#endif

#if RENDER_PASS == RENDER_PASS_EDITOR
  #define USE_DEBUG_INTERPOLATOR
#endif

#if INPUT_PIN_1_CONNECTED
#if !defined(USE_NORMAL)
  #define USE_NORMAL
#endif
#if !defined(USE_TANGENT)
  #define USE_TANGENT
#endif

#endif

#if INPUT_PIN_10_CONNECTED
  #define USE_OBJECT_POSITION_OFFSET
#endif

#if INPUT_PIN_11_CONNECTED
  #define USE_WORLD_POSITION_OFFSET
#endif

#include <Shaders/Materials/MaterialVertexShader.h>
#include <Shaders/Common/VisualShaderUtil.h>

VS_OUT main(VS_IN Input)
{
  return FillVertexData(Input);
}

#if defined(USE_OBJECT_POSITION_OFFSET)
float3 GetObjectPositionOffset(ezPerInstanceData data)
{
  return ToFloat3($in10);
}
#endif

#if defined(USE_WORLD_POSITION_OFFSET)
float3 GetWorldPositionOffset(ezPerInstanceData data, float3 worldPosition)
{
  return ToFloat3($in11);
}
#endif

" }

  string %CodeGeometryShader { "
#if VERTEX_SKINNING
  #define USE_SKINNING
#endif

#include <Shaders/Materials/MaterialStereoGeometryShader.h>

" }

  string %CodeMaterialParams { "
Permutation BLEND_MODE;
Permutation SHADING_MODE;
Permutation TWO_SIDED;
float MaskThreshold @Default($prop0);
" }

  string %CodePixelDefines { "
#define USE_SIMPLE_MATERIAL_MODEL
#define USE_MATERIAL_EMISSIVE
#define USE_MATERIAL_OCCLUSION
#define USE_TWO_SIDED_LIGHTING
#define USE_DECALS

#if VERTEX_SKINNING
  #define USE_SKINNING
#endif

#if RENDER_PASS == RENDER_PASS_EDITOR
  #define USE_DEBUG_INTERPOLATOR
#endif

#if $prop1
  #define USE_FOG
#endif

#if INPUT_PIN_1_CONNECTED
#if !defined(USE_NORMAL)
  #define USE_NORMAL
#endif
#if !defined(USE_TANGENT)
  #define USE_TANGENT
#endif
#endif

#if INPUT_PIN_8_CONNECTED
  #define USE_MATERIAL_REFRACTION
#endif

#if INPUT_PIN_9_CONNECTED
  #define USE_MATERIAL_SUBSURFACE_COLOR
#endif
" }

  string %CodePixelIncludes { "
#include <Shaders/Materials/MaterialPixelShader.h>
#include <Shaders/Common/VisualShaderUtil.h>
" }

  string %CodePixelSamplers { "" }
  string %CodePixelConstants { "" }
  string %CodeMaterialConstants { "

  FLOAT1(MaskThreshold);
  // Insert custom Visual Shader parameters here
  VSE_CONSTANTS
" }

  string %CodePixelBody { "
  
float3 GetBaseColor()
{
  return ToColor3($in0);
}

float3 GetNormal()
{
#if defined(USE_TANGENT) || defined(USE_NORMAL)
  return TangentToWorldSpace(ToFloat3($in1));
#else
  return ToFloat3($in1);
#endif
}

float GetMetallic()
{
  return saturate(ToFloat1($in2));
}

float GetReflectance()
{
  return saturate(ToFloat1($in3));
}

float GetRoughness()
{
  return saturate(ToFloat1($in4));
}

float GetOpacity()
{
  #if BLEND_MODE == BLEND_MODE_MASKED
    return saturate(ToFloat1($in5)) - GetMaterialData(MaskThreshold);
  #else
    return saturate(ToFloat1($in5));
  #endif
}

float3 GetEmissiveColor()
{
  return ToColor3($in6);
}

float GetOcclusion()
{
  return saturate(ToFloat1($in7));
}

#if defined USE_MATERIAL_REFRACTION
float4 GetRefractionColor()
{
  return ToColor4($in8);
}
#endif

#if defined USE_MATERIAL_SUBSURFACE_COLOR
float3 GetSubsurfaceColor()
{
  return ToColor3($in9);
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
    unsigned_int8 %Color { 200, 200, 200 }
    bool %Expose { true }
    string %DefaultValue { "1, 1, 1" }
  }

  // Pin 1
  InputPin %Normal
  {
    string %Type { "float3" }
    string %Color { "Violet" }
    string %DefaultValue { "float3(0, 0, 1)" }
    string %DefineWhenUsingDefaultValue { "USE_NORMAL" }
    string %Tooltip { "Surface normal in tangent space." }
  }

  // Pin 2
  InputPin %Metallic
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  // Pin 3
  InputPin %Reflectance
  {
    string %Type { "float" }
    string %Color { "Yellow" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }

  // Pin 4
  InputPin %Roughness
  {
    string %Type { "float" }
    string %Color { "Orange" }
    bool %Expose { true }
    string %DefaultValue { "0.5" }
  }

  // Pin 5
  InputPin %Opacity
  {
    string %Type { "float" }
    string %Color { "Red" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }

  // Pin 6
  InputPin %Emissive
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    bool %Expose { true }
    string %DefaultValue { "0" }
  }

  // Pin 7
  InputPin %Occlusion
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
  }
  
  // Pin 8
  InputPin %RefractionColor
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    bool %Expose { true }
    string %DefaultValue { "0, 0, 0, 1" }
  }

  // Pin 9
  InputPin %SubsurfaceColor
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    bool %Expose { true }
    string %DefaultValue { "0, 0, 0" }
  }

  // Pin 10
  InputPin %LocalPosOffset
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %DefaultValue { "0" }
  }

  // Pin 11
  InputPin %GlobalPosOffset
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %DefaultValue { "0" }
  }
}
