Node %SceneColor
{
  string %Category { "Utils" }
  unsigned_int8 %Color { 38, 105, 0 }
  
  InputPin %ScreenPosition
  {
    unsigned_int8 %Color { 50, 50, 128 }
    string %Type { "float2" }
    string %DefaultValue { "Input.Position.xy" }
  }

  OutputPin %Color
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 0, 0 }
    string %Inline { "SampleSceneColor(ToFloat2($in0))" }
  }
}

Node %SceneDepth
{
  string %Category { "Utils" }
  unsigned_int8 %Color { 38, 105, 0 }
  
  InputPin %ScreenPosition
  {
    unsigned_int8 %Color { 50, 50, 128 }
    string %Type { "float2" }
    string %DefaultValue { "Input.Position.xy" }
  }

  OutputPin %Depth
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 0, 0 }
    string %Inline { "SampleSceneDepth(ToFloat2($in0))" }
  }
}

Node %ScenePosition
{
  string %Category { "Utils" }
  unsigned_int8 %Color { 38, 105, 0 }
  
  InputPin %ScreenPosition
  {
    unsigned_int8 %Color { 50, 50, 128 }
    string %Type { "float2" }
    string %DefaultValue { "Input.Position.xy" }
  }

  OutputPin %Position
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 0, 0 }
    string %Inline { "SampleScenePosition(ToFloat2($in0))" }
  }
}

Node %DepthFade
{
  string %Category { "Utils" }
  unsigned_int8 %Color { 38, 105, 0 }
  
  InputPin %FadeDistance
  {
    unsigned_int8 %Color { 50, 50, 128 }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.0f" }
  }

  OutputPin %Fade
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 0, 0 }
    string %Inline { "DepthFade(Input.Position.xyw, ToFloat1($in0))" }
  }
}

Node %Fresnel
{
  string %Category { "Utils" }
  unsigned_int8 %Color { 38, 105, 0 }
  
  string %CodePixelBody { "

float VisualShaderFresnel(PS_IN Input, float3 normal, float f0, float exponent)
{
  float3 normalizedViewVector = normalize(GetCameraPosition() - Input.WorldPosition);
  float NdotV = saturate(dot(normalize(normal), normalizedViewVector));
  float f = pow(1 - NdotV, exponent);
  return f + (1 - f) * f0;
}

" }
  
  InputPin %Exponent
  {
    unsigned_int8 %Color { 50, 50, 128 }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "5.0f" }
  }
  
  InputPin %F0
  {
    unsigned_int8 %Color { 50, 50, 128 }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.04f" }
  }
  
  InputPin %Normal
  {
    unsigned_int8 %Color { 50, 50, 128 }
    string %Type { "float3" }
    string %DefaultValue { "GetNormal(Input)" }
  }

  OutputPin %Fresnel
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 0, 0 }
    string %Inline { "VisualShaderFresnel(Input, ToFloat3($in2), ToFloat1($in1), ToFloat1($in0))" }
  }
}

Node %Refraction
{
  string %Category { "Utils" }
  unsigned_int8 %Color { 38, 105, 0 }
  
  string %CodePixelBody { "

float4 VisualShaderRefraction(PS_IN Input, float3 worldNormal, float IoR, float thickness, float3 tintColor, float newOpacity)
{
  return CalculateRefraction(Input.WorldPosition, worldNormal, IoR, thickness, tintColor, newOpacity);
}

" }

  InputPin %Normal
  {
    unsigned_int8 %Color { 50, 50, 128 }
    string %Type { "float3" }
    string %DefaultValue { "GetNormal(Input)" }
  }
  
  InputPin %IoR
  {
    unsigned_int8 %Color { 50, 50, 128 }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.3f" }
  }
  
  InputPin %Thickness
  {
    unsigned_int8 %Color { 50, 50, 128 }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.0f" }
  }
  
  InputPin %TintColor
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 0, 0 }
    bool %Expose { true }
    string %DefaultValue { "1, 1, 1" }
  }
  
  InputPin %NewOpacity
  {
    unsigned_int8 %Color { 50, 50, 128 }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.0f" }
  }
  
  OutputPin %Refraction
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 0, 0 }
    string %Inline { "VisualShaderRefraction(Input, ToFloat3($in0), ToFloat1($in1), ToFloat1($in2), ToFloat3($in3), ToFloat1($in4))" }
  }
}
