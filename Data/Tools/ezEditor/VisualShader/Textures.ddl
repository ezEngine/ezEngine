Node %BaseTexture
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  string %Color { "Blue" }
  string %Docs { "Samples the material's base texture." }

  string %CodeMaterialParams { "Texture2D BaseTexture @Default(\"$prop0\");" }

  string %CodePixelSamplers { "
Texture2D BaseTexture BIND_GROUP(BG_MATERIAL);
SamplerState BaseTexture_AutoSampler BIND_GROUP(BG_MATERIAL);
" }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %DefaultValue { "RebeccaPurple.color" }
  }

  InputPin %UV
  {
    string %Color { "Teal" }
    string %Type { "float2" }
    string %DefaultValue { "G.Input.TexCoord0" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
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
    string %Color { "Red" }
    string %Inline { "BaseTexture.Sample(BaseTexture_AutoSampler, ToFloat2($in0)).x" }
  }

  OutputPin %Green
  {
    string %Type { "float" }
    string %Color { "Green" }
    string %Inline { "BaseTexture.Sample(BaseTexture_AutoSampler, ToFloat2($in0)).y" }
  }

  OutputPin %Blue
  {
    string %Type { "float" }
    string %Color { "Blue" }
    string %Inline { "BaseTexture.Sample(BaseTexture_AutoSampler, ToFloat2($in0)).z" }
  }

  OutputPin %Alpha
  {
    string %Type { "float" }
    string %Inline { "BaseTexture.Sample(BaseTexture_AutoSampler, ToFloat2($in0)).w" }
  }
}

Node %NormalTexture
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  string %Color { "Blue" }
  string %Docs { "Samples the material's normal map." }

  string %CodeMaterialParams { "
Texture2D $prop0 @Default(\"$prop1\");
" }

  string %CodePixelSamplers { "
Texture2D $prop0 BIND_GROUP(BG_MATERIAL);
SamplerState $prop0_AutoSampler BIND_GROUP(BG_MATERIAL);
" }

  Property %Name
  {
    string %Type { "identifier" }
    string %DefaultValue { "NormalTexture" }
  }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %DefaultValue {"{ 4dc82890-39e3-4bfc-a97d-86a984d4d3db }" }// Neutral normal map
  }

  InputPin %UV
  {
    string %Type { "float2" }
    string %Color { "Teal" }
    string %DefaultValue { "G.Input.TexCoord0" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
    string %Tooltip { "Optional UV coordinates to sample the texture. Default uses the mesh UV coordinates." }
  }

  OutputPin %Normal
  {
    string %Type { "float3" }
    string %Color { "Violet" }
    string %Inline { "DecodeNormalTexture($prop0.Sample($prop0_AutoSampler, ToFloat2($in0)))" }
    string %Tooltip { "Normal in Tangent Space" }
  }
}

Node %EmissiveTexture
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  string %Color { "Blue" }
  string %Docs { "Samples the material's emissive texture." }

  string %CodeMaterialParams {"Texture2D EmissiveTexture @Default(\"$prop0\"); " }

  string %CodePixelSamplers { "
Texture2D EmissiveTexture BIND_GROUP(BG_MATERIAL);
SamplerState EmissiveTexture_AutoSampler BIND_GROUP(BG_MATERIAL);
" }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %DefaultValue { "" }
  }

  InputPin %UV
  {
    string %Type { "float2" }
    string %Color { "Teal" }
    string %DefaultValue { "G.Input.TexCoord0" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
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
    string %Color { "Red" }
    string %Inline { "EmissiveTexture.Sample(EmissiveTexture_AutoSampler, ToFloat2($in0)).x" }
  }

  OutputPin %Green
  {
    string %Type { "float" }
    string %Color { "Green" }
    string %Inline { "EmissiveTexture.Sample(EmissiveTexture_AutoSampler, ToFloat2($in0)).y" }
  }

  OutputPin %Blue
  {
    string %Type { "float" }
    string %Color { "Blue" }
    string %Inline { "EmissiveTexture.Sample(EmissiveTexture_AutoSampler, ToFloat2($in0)).z" }
  }
}

Node %MetallicTexture
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  string %Color { "Blue" }
  string %Docs { "Samples the material's metalness texture." }

  string %CodeMaterialParams { "Texture2D MetallicTexture @Default(\"$prop0\"); " }

  string %CodePixelSamplers { "
Texture2D MetallicTexture BIND_GROUP(BG_MATERIAL);
SamplerState MetallicTexture_AutoSampler BIND_GROUP(BG_MATERIAL);
" }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %DefaultValue { "" }
  }

  InputPin %UV
  {
    string %Type { "float2" }
    string %Color { "Teal" }
    string %DefaultValue { "G.Input.TexCoord0" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
    string %Tooltip { "Optional UV coordinates to sample the texture. Default uses the mesh UV coordinates." }
  }

  OutputPin %Metallic
  {
    string %Type { "float" }
    string %Inline { "MetallicTexture.Sample(MetallicTexture_AutoSampler, ToFloat2($in0)).x" }
    string %Tooltip { "Outputs only the red component of the sampled texture." }
  }
}

Node %RoughnessTexture
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  string %Color { "Blue" }
  string %Docs { "Samples the material's roughness texture." }

  string %CodeMaterialParams { "Texture2D RoughnessTexture @Default(\"$prop0\"); " }

  string %CodePixelSamplers { "
Texture2D RoughnessTexture BIND_GROUP(BG_MATERIAL);
SamplerState RoughnessTexture_AutoSampler BIND_GROUP(BG_MATERIAL);
" }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %DefaultValue { "" }
  }

  InputPin %UV
  {
    string %Type { "float2" }
    string %Color { "Teal" }
    string %DefaultValue { "G.Input.TexCoord0" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
    string %Tooltip { "Optional UV coordinates to sample the texture. Default uses the mesh UV coordinates." }
  }

  OutputPin %Roughness
  {
    string %Type { "float" }
    string %Color { "Orange" }
    string %Inline { "RoughnessTexture.Sample(RoughnessTexture_AutoSampler, ToFloat2($in0)).x" }
    string %Tooltip { "Outputs only the red component of the sampled texture." }
  }
}



Node %Texture2D
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  string %Color { "Blue" }
  string %Docs { "Samples a custom 2D texture." }

  string %CodeMaterialParams { "
Texture2D $prop0 @Default(\"$prop1\");
" }

  string %CodePixelSamplers { "
Texture2D $prop0 BIND_GROUP(BG_MATERIAL);
SamplerState $prop0_AutoSampler BIND_GROUP(BG_MATERIAL);
" }

  Property %Name
  {
    string %Type { "identifier" }
    string %DefaultValue { "CustomTexture" }
  }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %DefaultValue { "" }
  }

  InputPin %UV
  {
    string %Type { "float2" }
    string %Color { "Teal" }
    string %DefaultValue { "G.Input.TexCoord0" }
    string %DefineWhenUsingDefaultValue { "USE_TEXCOORD0" }
    string %Tooltip { "Optional UV coordinates to sample the texture. Default uses the mesh UV coordinates." }
  }

  InputPin %Sampler
  {
    string %Type { "sampler" }
    string %Color { "Cyan" }
    string %DefaultValue { "$prop0_AutoSampler" }
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
    string %Color { "Red" }
    string %Inline { "$prop0.Sample($in1, ToFloat2($in0)).x" }
  }

  OutputPin %Green
  {
    string %Type { "float" }
    string %Color { "Green" }
    string %Inline { "$prop0.Sample($in1, ToFloat2($in0)).y" }
  }

  OutputPin %Blue
  {
    string %Type { "float" }
    string %Color { "Blue" }
    string %Inline { "$prop0.Sample($in1, ToFloat2($in0)).z" }
  }

  OutputPin %Alpha
  {
    string %Type { "float" }
    string %Inline { "$prop0.Sample($in1, ToFloat2($in0)).w" }
  }
}

Node %Texture3Way
{
  string %Category { "Texturing" }
  string %NodeType { "Texture" }
  string %Color { "Blue" }
  string %Docs { "Samples a custom 2D texture 3 times and projects it from 3 sides according to the given normal." }

  string %CodeMaterialParams { "
Texture2D $prop0 @Default(\"$prop1\");
" }

  string %CodePixelSamplers { "
Texture2D $prop0 BIND_GROUP(BG_MATERIAL);
SamplerState $prop0_AutoSampler BIND_GROUP(BG_MATERIAL);
" }

  Property %Name
  {
    string %Type { "identifier" }
    string %DefaultValue { "CustomTexture" }
  }

  Property %Texture
  {
    string %Type { "Texture2D" }
    string %DefaultValue {"" }
  }

  Property %Tiling
  {
    string %Type { "float" }
    string %DefaultValue { "1" }
  }

  InputPin %WorldPosition
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %DefaultValue { "G.Input.WorldPosition" }
  }

  InputPin %WorldNormal
  {
    string %Type { "float3" }
    string %Color { "Violet" }
    string %DefaultValue { "G.Input.Normal" }
    string %DefineWhenUsingDefaultValue { "USE_NORMAL" }
  }

  OutputPin %RGBA
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "SampleTexture3Way($prop0, $prop0_AutoSampler, $in1, $in0, $prop2)" }
  }
}

Node %BlendNormals
{
  string %Category { "Texturing" }
  string %Color { "Violet" }
  string %Docs { "Blends a detail normal on top of a base normal." }

  InputPin %BaseNormal
  {
    string %Type { "float3" }
    string %Color { "Violet" }
  }

  InputPin %DetailNormal
  {
    string %Type { "float3" }
    string %Color { "Violet" }
  }

  OutputPin %Normal
  {
    string %Type { "float3" }
    string %Color { "Violet" }
    string %Inline { "BlendNormals($in0, $in1)" }
    string %Tooltip { "Blended Normal" }
  }
}
