Node %SceneColor
{
  string %Category { "Utils" }
  string %Color { "Green" }
  
  InputPin %ScreenPosition
  {
    string %Type { "float2" }
    string %DefaultValue { "G.Input.Position.xy" }
  }

  OutputPin %Color
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "SampleSceneColor(ToFloat2($in0))" }
  }
}

Node %SceneDepth
{
  string %Category { "Utils" }
  string %Color { "Green" }
  
  InputPin %ScreenPosition
  {
    string %Type { "float2" }
    string %DefaultValue { "G.Input.Position.xy" }
  }

  OutputPin %Depth
  {
    string %Type { "float3" }
    string %Inline { "SampleSceneDepth(ToFloat2($in0))" }
  }
}

Node %ScenePosition
{
  string %Category { "Utils" }
  string %Color { "Green" }
  
  InputPin %ScreenPosition
  {
    string %Type { "float2" }
    string %DefaultValue { "G.Input.Position.xy" }
  }

  OutputPin %Position
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "SampleScenePosition(ToFloat2($in0))" }
  }
}

Node %DepthFade
{
  string %Category { "Utils" }
  string %Color { "Green" }
  
  InputPin %FadeDistance
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.0f" }
  }

  OutputPin %Fade
  {
    string %Type { "float" }
    string %Inline { "DepthFade(G.Input.Position.xyw, ToFloat1($in0))" }
  }
}

Node %Fresnel
{
  string %Category { "Utils" }
  string %Color { "Green" }
  
  string %CodePixelBody { "

float VisualShaderFresnel(float3 normal, float f0, float exponent)
{
  float3 normalizedViewVector = normalize(GetCameraPosition() - G.Input.WorldPosition);
  float NdotV = saturate(dot(normalize(normal), normalizedViewVector));
  float f = pow(1 - NdotV, exponent);
  return f + (1 - f) * f0;
}

" }
  
  InputPin %Exponent
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "5.0f" }
  }
  
  InputPin %F0
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.04f" }
  }
  
  InputPin %Normal
  {
    string %Type { "float3" }
    string %DefaultValue { "GetNormal()" }
  }

  OutputPin %Fresnel
  {
    string %Type { "float" }
    string %Inline { "VisualShaderFresnel(ToFloat3($in2), ToFloat1($in1), ToFloat1($in0))" }
  }
}

Node %Refraction
{
  string %Category { "Utils" }
  string %Color { "Green" }
  
  string %CodePixelBody { "

float4 VisualShaderRefraction(float3 worldNormal, float IoR, float thickness, float3 tintColor, float newOpacity)
{
  return CalculateRefraction(G.Input.WorldPosition, worldNormal, IoR, thickness, tintColor, newOpacity);
}

" }

  InputPin %Normal
  {
    string %Color { "Violet" }
    string %Type { "float3" }
    string %DefaultValue { "GetNormal()" }
  }
  
  InputPin %IoR
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.3f" }
  }
  
  InputPin %Thickness
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.0f" }
  }
  
  InputPin %TintColor
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    bool %Expose { true }
    string %DefaultValue { "1, 1, 1" }
  }
  
  InputPin %NewOpacity
  {
    string %Color { "Red" }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.0f" }
  }
  
  OutputPin %Refraction
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "VisualShaderRefraction(ToFloat3($in0), ToFloat1($in1), ToFloat1($in2), ToFloat3($in3), ToFloat1($in4))" }
  }
}
