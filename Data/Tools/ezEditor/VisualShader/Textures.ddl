Node %BaseTexture
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  unsigned_int8 %Color { 0, 89, 153 }
  string %CodeMaterialParams { "Texture2D BaseTexture @Default(\"$prop0\");" }

  string %CodePixelSamplers { "
Texture2D BaseTexture;
SamplerState BaseTexture_AutoSampler;
" }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %Value { "RebeccaPurple.color" }
  }

  InputPin %UV
  {
    unsigned_int8 %Color { 50, 50, 128 }
    string %Type { "float2" }
    string %Fallback { "Input.TexCoords" }
    string %Tooltip { "Optional UV coordinates to sample the texture. Default uses the mesh UV coordinates." }
  }

  OutputPin %RGBA
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "BaseTexture.Sample(BaseTexture_AutoSampler, ToFloat2($in0))" }
  }

  OutputPin %Red
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 0, 0 }
    string %Inline { "BaseTexture.Sample(BaseTexture_AutoSampler, ToFloat2($in0)).x" }
  }

  OutputPin %Green
  {
    string %Type { "float" }
    unsigned_int8 %Color { 0, 128, 0 }
    string %Inline { "BaseTexture.Sample(BaseTexture_AutoSampler, ToFloat2($in0)).y" }
  }

  OutputPin %Blue
  {
    string %Type { "float" }
    unsigned_int8 %Color { 0, 0, 128 }
    string %Inline { "BaseTexture.Sample(BaseTexture_AutoSampler, ToFloat2($in0)).z" }
  }

  OutputPin %Alpha
  {
    string %Type { "float" }
    unsigned_int8 %Color { 175, 175, 117 }
    string %Inline { "BaseTexture.Sample(BaseTexture_AutoSampler, ToFloat2($in0)).w" }
  }
}

Node %NormalTexture
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  unsigned_int8 %Color { 0, 89, 153 }
  string %CodeMaterialParams { "Texture2D NormalTexture @Default(\"$prop0\");" }

  string %CodePixelSamplers { "
Texture2D NormalTexture;
SamplerState NormalTexture_AutoSampler;
" }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %Value {"{ 4dc82890-39e3-4bfc-a97d-86a984d4d3db }" }// Neutral normal map
  }

  InputPin %UV
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 50, 50, 128 }
    string %Fallback { "Input.TexCoords" }
    string %Tooltip { "Optional UV coordinates to sample the texture. Default uses the mesh UV coordinates." }
  }

  OutputPin %Normal
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 128, 128, 255 }
    string %Inline { "normalize(NormalTexture.Sample(NormalTexture_AutoSampler, ToFloat2($in0)).xyz * 2.0 - 1.0)" }
    string %Tooltip { "Normal in Tangent Space" }
  }
}

Node %EmissiveTexture
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  unsigned_int8 %Color { 0, 89, 153 }
  string %CodeMaterialParams {"Texture2D EmissiveTexture @Default(\"$prop0\"); " }

  string %CodePixelSamplers { "
Texture2D EmissiveTexture;
SamplerState EmissiveTexture_AutoSampler;
" }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %Value { "" }
  }

  InputPin %UV
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 50, 50, 128 }
    string %Fallback { "Input.TexCoords" }
    string %Tooltip { "Optional UV coordinates to sample the texture. Default uses the mesh UV coordinates." }
  }

  OutputPin %RGBA
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "EmissiveTexture.Sample(EmissiveTexture_AutoSampler, ToFloat2($in0))" }
  }

  OutputPin %Red
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 0, 0 }
    string %Inline { "EmissiveTexture.Sample(EmissiveTexture_AutoSampler, ToFloat2($in0)).x" }
  }

  OutputPin %Green
  {
    string %Type { "float" }
    unsigned_int8 %Color { 0, 128, 0 }
    string %Inline { "EmissiveTexture.Sample(EmissiveTexture_AutoSampler, ToFloat2($in0)).y" }
  }

  OutputPin %Blue
  {
    string %Type { "float" }
    unsigned_int8 %Color { 0, 0, 128 }
    string %Inline { "EmissiveTexture.Sample(EmissiveTexture_AutoSampler, ToFloat2($in0)).z" }
  }
}

Node %MetallicTexture
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  unsigned_int8 %Color { 0, 89, 153 }
  string %CodeMaterialParams { "Texture2D MetallicTexture @Default(\"$prop0\"); " }

  string %CodePixelSamplers { "
Texture2D MetallicTexture;
SamplerState MetallicTexture_AutoSampler;
" }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %Value { "" }
  }

  InputPin %UV
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 50, 50, 128 }
    string %Fallback { "Input.TexCoords" }
    string %Tooltip { "Optional UV coordinates to sample the texture. Default uses the mesh UV coordinates." }
  }

  OutputPin %Metallic
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 128, 128 }
    string %Inline { "MetallicTexture.Sample(MetallicTexture_AutoSampler, ToFloat2($in0)).x" }
    string %Tooltip { "Outputs only the red component of the sampled texture." }
  }
}

Node %RoughnessTexture
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  unsigned_int8 %Color { 0, 89, 153 }
  string %CodeMaterialParams { "Texture2D RoughnessTexture @Default(\"$prop0\"); " }

  string %CodePixelSamplers { "
Texture2D RoughnessTexture;
SamplerState RoughnessTexture_AutoSampler;
" }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %Value { "" }
  }

  InputPin %UV
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 50, 50, 128 }
    string %Fallback { "Input.TexCoords" }
    string %Tooltip { "Optional UV coordinates to sample the texture. Default uses the mesh UV coordinates." }
  }

  OutputPin %Roughness
  {
    string %Type { "float" }
    unsigned_int8 %Color { 150, 64, 64 }
    string %Inline { "RoughnessTexture.Sample(RoughnessTexture_AutoSampler, ToFloat2($in0)).x" }
    string %Tooltip { "Outputs only the red component of the sampled texture." }
  }
}



Node %Texture2D
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  unsigned_int8 %Color { 0, 89, 153 }
  string %CodeMaterialParams { "
Texture2D $prop0 @Default(\"$prop1\");
" }

  string %CodePixelSamplers { "
Texture2D $prop0;
SamplerState $prop0_AutoSampler;
" }

  Property %Name
  {
    string %Type { "string" }
    string %Value { "" }
  }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %Value { "" }
  }

  InputPin %UV
  {
    string %Type { "float2" }
    unsigned_int8 %Color { 50, 50, 128 }
    string %Fallback { "Input.TexCoords" }
    string %Tooltip { "Optional UV coordinates to sample the texture. Default uses the mesh UV coordinates." }
  }

  InputPin %Sampler
  {
    string %Type { "sampler" }
    unsigned_int8 %Color { 0, 96, 96 }
    string %Fallback { "$prop0_AutoSampler" }
    string %Tooltip { "Optional sampler state to use." }
  }

  OutputPin %RGBA
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "$prop0.Sample($in1, ToFloat2($in0))" }
  }

  OutputPin %Red
  {
    string %Type { "float" }
    unsigned_int8 %Color { 128, 0, 0 }
    string %Inline { "$prop0.Sample($in1, ToFloat2($in0)).x" }
  }

  OutputPin %Green
  {
    string %Type { "float" }
    unsigned_int8 %Color { 0, 128, 0 }
    string %Inline { "$prop0.Sample($in1, ToFloat2($in0)).y" }
  }

  OutputPin %Blue
  {
    string %Type { "float" }
    unsigned_int8 %Color { 0, 0, 128 }
    string %Inline { "$prop0.Sample($in1, ToFloat2($in0)).z" }
  }

  OutputPin %Alpha
  {
    string %Type { "float" }
    unsigned_int8 %Color { 175, 175, 117 }
    string %Inline { "$prop0.Sample($in1, ToFloat2($in0)).w" }
  }
}
