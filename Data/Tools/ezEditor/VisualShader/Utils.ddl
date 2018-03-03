Node %DepthFade
{
  string %Category { "Utils" }
  unsigned_int8 %Color { 38, 105, 0 }
  
  InputPin %FadeDistance
  {
    unsigned_int8 %Color { 50, 50, 128 }
    string %Type { "float" }
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
    string %DefaultValue { "5.0f" }
  }
  
  InputPin %F0
  {
    unsigned_int8 %Color { 50, 50, 128 }
    string %Type { "float" }
    string %DefaultValue { "0.04f" }
  }
  
  InputPin %Normal
  {
    unsigned_int8 %Color { 50, 50, 128 }
    string %Type { "float" }
    string %DefaultValue { "GetNormal(Input)" }
  }

  OutputPin %Fresnel
  {
    string %Type { "float" }
    unsigned_int8 %Color { 200, 0, 0 }
    string %Inline { "VisualShaderFresnel(Input, ToFloat3($in2), ToFloat1($in1), ToFloat1($in0))" }
  }
}
