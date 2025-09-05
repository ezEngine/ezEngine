Node %SceneColor
{
  string %Category { "Utils" }
  string %Color { "Green" }
  string %Docs { "Samples and outputs the color of the rendered scene at a specified screen-space position.\nUseful for effects that require access to the underlying scene color, such as post-processing or custom blending." }

  InputPin %ScreenPosition
  {
    string %Type { "float2" }
    string %DefaultValue { "G.Input.Position.xy" }
    string %Tooltip { "Screen-space position (X, Y) to sample the scene color from." }
  }

  OutputPin %Color
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "SampleSceneColor(ToFloat2($in0))" }
    string %Tooltip { "The color sampled from the scene at the given screen-space position." }
  }
}

Node %SceneDepth
{
  string %Category { "Utils" }
  string %Color { "Green" }
  string %Docs { "Samples and outputs the depth value from the scene at a specified screen-space position.\nThis can be used for depth-based effects, such as fog, fading, or custom depth comparisons." }

  InputPin %ScreenPosition
  {
    string %Type { "float2" }
    string %DefaultValue { "G.Input.Position.xy" }
    string %Tooltip { "Screen-space position (X, Y) to sample the scene depth from." }
  }

  OutputPin %Depth
  {
    string %Type { "float3" }
    string %Inline { "SampleSceneDepth(ToFloat2($in0))" }
    string %Tooltip { "The depth value sampled from the scene at the given screen-space position." }
  }
}

Node %ScenePosition
{
  string %Category { "Utils" }
  string %Color { "Green" }
  string %Docs { "Outputs the world-space position corresponding to a given screen-space position. Useful for reconstructing 3D positions from screen coordinates, enabling advanced effects like screen-space reflections or custom geometry manipulation." }

  InputPin %ScreenPosition
  {
    string %Type { "float2" }
    string %DefaultValue { "G.Input.Position.xy" }
    string %Tooltip { "Screen-space position (X, Y) to sample the world position from." }
  }

  OutputPin %Position
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "SampleScenePosition(ToFloat2($in0))" }
    string %Tooltip { "The world-space position at the given screen-space position." }
  }
}

Node %DepthFade
{
  string %Category { "Utils" }
  string %Color { "Green" }
  string %Docs { "Calculates a fade factor for the current pixel based on its proximity to scene geometry.\nCommonly used to smoothly fade out objects as they approach other geometry, preventing hard intersections and improving visual quality." }
 
  InputPin %FadeDistance
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.0f" }
    string %Tooltip { "Distance over which the fade effect occurs near scene geometry." }
  }

  OutputPin %Fade
  {
    string %Type { "float" }
    string %Inline { "DepthFade(G.Input.Position.xyw, ToFloat1($in0))" }
    string %Tooltip { "Fade value (0-1) based on proximity to scene geometry." }
  }
}

Node %Fresnel
{
  string %Category { "Utils" }
  string %Color { "Green" }
  string %Docs { "Calculates the Fresnel effect, which simulates how the reflectivity of a surface changes depending on the viewing angle.\nThis is essential for realistic rendering of materials like glass, water, and metals, where edges appear more reflective." }
  
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
    string %Tooltip { "Controls the sharpness of the Fresnel effect. Higher values make the edge highlight more pronounced." }
  }
  
  InputPin %F0
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.04f" }
    string %Tooltip { "Base reflectance at normal incidence. Typical values: 0.04 for non-metals, higher for metals." }
  }
  
  InputPin %Normal
  {
    string %Type { "float3" }
    string %DefaultValue { "GetNormal()" }
    string %Tooltip { "Surface normal used for Fresnel calculation." }
  }

  OutputPin %Fresnel
  {
    string %Type { "float" }
    string %Inline { "VisualShaderFresnel(ToFloat3($in2), ToFloat1($in1), ToFloat1($in0))" }
    string %Tooltip { "Fresnel effect value, highlighting edges based on viewing angle." }
  }
}

Node %Refraction
{
  string %Category { "Utils" }
  string %Color { "Green" }
  string %Docs { "Simulates the bending of light as it passes through a transparent or semi-transparent material, based on the surface normal, index of refraction, thickness, and tint.\nOutputs the resulting color and opacity, enabling realistic glass, water, or crystal effects." }

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
    string %Tooltip { "Surface normal used to calculate refraction direction." }
  }
  
  InputPin %IoR
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.3f" }
    string %Tooltip { "Index of Refraction. Controls how much light bends when passing through the material." }
  }
  
  InputPin %Thickness
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.0f" }
    string %Tooltip { "Thickness of the refractive object. Affects distortion and color absorption." }
  }
  
  InputPin %TintColor
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    bool %Expose { true }
    string %DefaultValue { "1, 1, 1" }
    string %Tooltip { "Color tint applied to refracted light as it passes through the material." }
  }
  
  InputPin %NewOpacity
  {
    string %Color { "Red" }
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1.0f" }
    string %Tooltip { "Opacity of the refracted result. Controls transparency." }
  }
  
  OutputPin %Refraction
  {
    string %Type { "float4" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "VisualShaderRefraction(ToFloat3($in0), ToFloat1($in1), ToFloat1($in2), ToFloat3($in3), ToFloat1($in4))" }
    string %Tooltip { "Resulting color and opacity after applying refraction, simulating light bending through the material." }
  }
}
