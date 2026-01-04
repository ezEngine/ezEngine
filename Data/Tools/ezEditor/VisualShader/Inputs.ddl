Node %Time
{
  string %Category { "Input" }
  string %Color { "Green" }
  string %Docs { "Outputs time values from different clocks. " }

  OutputPin %Global
  {
    string %Type { "float" }
    string %Inline { "GlobalTime" }
    string %Tooltip { "Real time. Always at the same speed, unaffected by world simulation speed." }
  }

  OutputPin %World
  {
    string %Type { "float" }
    string %Inline { "WorldTime" }
    string %Tooltip { "World simulation time. Affected by simulation speed (slow-motion) and world paused state." }
  }
}

Node %UV
{
  string %Category { "Input" }
  string %Color { "Teal" }
  string %Docs { "Outputs the first vertex texture coordinate (UV0). " }

  string %CodeShaderShared { "
#ifndef USE_TEXCOORD0
  #define USE_TEXCOORD0
#endif
" }

  OutputPin %UV
  {
    string %Type { "float2" }
    string %Color { "Teal" }
    string %Inline { "G.Input.TexCoord0" }
    string %Tooltip { "The UV 0 texture coordinate." }
  }
}

Node %UV2
{
  string %Category { "Input" }
  string %Color { "Teal" }
  string %Docs { "Outputs the second vertex texture coordinate (UV1). " }

  string %CodeShaderShared { "
#ifndef USE_TEXCOORD0
  #define USE_TEXCOORD0
#endif
#ifndef USE_TEXCOORD1
  #define USE_TEXCOORD1
#endif
" }

  OutputPin %UV
  {
    string %Type { "float2" }
    string %Color { "Teal" }
    string %Inline { "G.Input.TexCoord1" }
    string %Tooltip { "The UV 0 texture coordinate." }
  }
}

Node %UV_Scroll
{
  string %Category { "Input" }
  string %Color { "Green" }
  string %Docs { "Outputs the first vertex texture coordinate (UV0) and applies a scrolling effect, using the world time. " }
  string %Title { "UV Scroll: {$in1}" }

  string %CodeShaderShared { "
#ifndef USE_TEXCOORD0
  #define USE_TEXCOORD0
#endif
" }

  InputPin %Speed
  {
    string %Type { "float2" }
    bool %Expose { true }
    string %DefaultValue { "1, 1" }
  }

  InputPin %Scale
  {
    string %Type { "float2" }
    bool %Expose { true }
    string %DefaultValue { "1, 1" }
  }

  InputPin %Offset
  {
    string %Type { "float2" }
    bool %Expose { true }
    string %DefaultValue { "0, 0" }
  }

  OutputPin %UV
  {
    string %Type { "float2" }
    string %Color { "Teal" }
    string %Inline { "(G.Input.TexCoord0 * $in1 + $in2) + frac(WorldTime * $in0)" }
    string %Tooltip { "The scrolled UV 0 texture coordinate." }
  }
}

Node %VertexPosition
{
  string %Category { "Input" }
  string %Color { "Green" }
  string %Docs { "Outputs the vertex position data.\nFor vertex shaders this is the untransformed local position, for pixel shaders it is the transformed position." }

  OutputPin %Position
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "G.Input.Position" }
    string %Tooltip { "The vertex position. For vertex shaders this is the local position, for pixel shaders it is the transformed position." }
  }

  OutputPin %X
  {
    string %Type { "float" }
    string %Color { "Red" }
    string %Inline { "G.Input.Position.x" }
  }

  OutputPin %Y
  {
    string %Type { "float" }
    string %Color { "Green" }
    string %Inline { "G.Input.Position.y" }
  }

  OutputPin %Z
  {
    string %Type { "float" }
    string %Color { "Blue" }
    string %Inline { "G.Input.Position.z" }
  }
}

Node %VertexWorldPosition
{
  string %Category { "Input" }
  string %Color { "Green" }
  string %Docs { "Outputs the vertex position transformed into world-space." }

  OutputPin %Position
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "G.Input.WorldPosition" }
    string %Tooltip { "The vertex world space position." }
  }

  OutputPin %X
  {
    string %Type { "float" }
    string %Color { "Red" }
    string %Inline { "G.Input.WorldPosition.x" }
  }

  OutputPin %Y
  {
    string %Type { "float" }
    string %Color { "Green" }
    string %Inline { "G.Input.WorldPosition.y" }
  }

  OutputPin %Z
  {
    string %Type { "float" }
    string %Color { "Blue" }
    string %Inline { "G.Input.WorldPosition.z" }
  }
}

Node %VertexNormal
{
  string %Category { "Input" }
  string %Color { "Green" }
  string %Docs { "Outputs the vertex normal.\nFor vertex shaders this is in local space, for pixel shaders it is in world space." }

  string %CodeShaderShared { "
#ifndef USE_NORMAL
  #define USE_NORMAL
#endif
" }

  OutputPin %Normal
  {
    string %Type { "float3" }
    string %Color { "Violet" }
    string %Inline { "G.Input.Normal" }
    string %Tooltip { "The vertex normal. For vertex shaders this is in local space, for pixel shaders it is in world space." }
  }

  OutputPin %X
  {
    string %Type { "float" }
    string %Color { "Red" }
    string %Inline { "G.Input.Normal.x" }
  }

  OutputPin %Y
  {
    string %Type { "float" }
    string %Color { "Green" }
    string %Inline { "G.Input.Normal.y" }
  }

  OutputPin %Z
  {
    string %Type { "float" }
    string %Color { "Blue" }
    string %Inline { "G.Input.Normal.z" }
  }
}

Node %VertexTangent
{
  string %Category { "Input" }
  string %Color { "Green" }
  string %Docs { "Outputs the vertex tangent.\nFor vertex shaders this is in local space, for pixel shaders it is in world space." }

  string %CodeShaderShared { "
#ifndef USE_TANGENT
  #define USE_TANGENT
#endif
" }

  OutputPin %Tangent
  {
    string %Type { "float3" }
    string %Color { "Grape" }
    string %Inline { "G.Input.Tangent" }
    string %Tooltip { "The vertex tangent. For vertex shaders this is the local tangent, for pixel shaders it is the transformed tangent." }
  }
}

Node %VertexColor
{
  string %Category { "Input" }
  string %Color { "Green" }
  string %Docs { "Outputs the first vertex color value (Color0)." }

  string %CodeShaderShared { "
#ifndef USE_COLOR0
  #define USE_COLOR0
#endif
" }

  OutputPin %Color
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "G.Input.Color0" }
    string %Tooltip { "The vertex color" }
  }

  OutputPin %Red
  {
    string %Type { "float" }
    string %Color { "Red" }
    string %Inline { "G.Input.Color0.x" }
  }

  OutputPin %Green
  {
    string %Type { "float" }
    string %Color { "Green" }
    string %Inline { "G.Input.Color0.y" }
  }

  OutputPin %Blue
  {
    string %Type { "float" }
    string %Color { "Blue" }
    string %Inline { "G.Input.Color0.z" }
  }

  OutputPin %Alpha
  {
    string %Type { "float" }
    string %Inline { "G.Input.Color0.w" }
  }
}

Node %VertexColor2
{
  string %Category { "Input" }
  string %Color { "Green" }
  string %Docs { "Outputs the second vertex color value (Color1)." }

  string %CodeShaderShared { "
#ifndef USE_COLOR0
  #define USE_COLOR0
#endif
#ifndef USE_COLOR1
  #define USE_COLOR1
#endif
" }

  OutputPin %Color
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "G.Input.Color1" }
    string %Tooltip { "The second vertex color" }
  }

  OutputPin %Red
  {
    string %Type { "float" }
    string %Color { "Red" }
    string %Inline { "G.Input.Color0.x" }
  }

  OutputPin %Green
  {
    string %Type { "float" }
    string %Color { "Green" }
    string %Inline { "G.Input.Color0.y" }
  }

  OutputPin %Blue
  {
    string %Type { "float" }
    string %Color { "Blue" }
    string %Inline { "G.Input.Color0.z" }
  }

  OutputPin %Alpha
  {
    string %Type { "float" }
    string %Inline { "G.Input.Color0.w" }
  }
}

Node %InstanceData
{
  string %Category { "Input" }
  string %Color { "Green" }
  string %Docs { "Outputs the object instance specific data." }

  OutputPin %Color
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "GetInstanceData().Color" }
    string %Tooltip { "Per instance color." }
  }
  
  OutputPin %CustomData
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "GetInstanceData().CustomData" }
    string %Tooltip { "Per instance custom data" }
  }
}

Node %Camera
{
  string %Category { "Input" }
  string %Color { "Green" }
  string %Docs { "Outputs data about the rendering camera." }

  OutputPin %Position
  {
    string %Type { "float3" }
	string %Color { "Indigo" }
    string %Inline { "GetCameraPosition()" }
    string %Tooltip { "Global camera position." }
  }

  OutputPin %Forwards
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "GetCameraDirForwards()" }
    string %Tooltip { "Forward direction vector of the camera." }
  }

  OutputPin %Right
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "GetCameraDirRight()" }
    string %Tooltip { "Right direction vector of the camera." }
  }

  OutputPin %Up
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "GetCameraDirUp()" }
    string %Tooltip { "Up direction vector of the camera." }
  }

  OutputPin %Exposure
  {
    string %Type { "float" }
    string %Inline { "Exposure" }
    string %Tooltip { "Current exposure value of the camera." }
  }
}

Node %ScreenCoord
{
  string %Category { "Input" }
  string %Color { "Green" }
  string %Docs { "Outputs the integer screen coordinate of the pixel. Values are in range 0 to resolution x/y." }

  OutputPin %Position
  {
    string %Type { "float2" }
    string %Color { "Indigo" }
    string %Inline { "G.Input.Position.xy" }
    string %Tooltip { "The integer screen coordiante of the pixel from 0 to resolution x/y." }
  }
}
