Node %ScrollFloat4
{
  string %Category { "Utils" }
  string %Color { "Green" }
  string %Docs { "Scrolls a float4 value (e.g. a UV coordinate) over time, using the world time.\nThe speed is rounded to a multiple of 0.001 (see 'Wrap Safe Rate'), so the scrolling does not jump when the shader time wraps around." }
  string %Title { "Scroll: {$in0}" }

  InputPin %UV
  {
    string %Type { "float4" }
    string %DefaultValue { "G.Input.TexCoord0" }
    string %Tooltip { "The float4 value to scroll, e.g. a UV coordinate." }
  }

  InputPin %Speed
  {
    string %Type { "float4" }
    bool %Expose { true }
    string %DefaultValue { "1, 1, 1, 1" }
    string %Tooltip { "Scroll speed along X, Y, Z and W. Rounded to a multiple of 0.001." }
  }

  InputPin %Scale
  {
    string %Type { "float4" }
    bool %Expose { true }
    string %DefaultValue { "1, 1, 1, 1" }
    string %Tooltip { "Scale applied to the input value before scrolling." }
  }

  InputPin %Offset
  {
    string %Type { "float4" }
    bool %Expose { true }
    string %DefaultValue { "0, 0, 0, 0" }
    string %Tooltip { "Constant offset applied after scaling." }
  }

  OutputPin %UV
  {
    string %Type { "float4" }
    string %Color { "Teal" }
    string %Inline { "(ToFloat4Direction($in0) * $in2 + $in3) + frac(WorldTime * WrapSafeRate($in1, 1.0))" }
    string %Tooltip { "The scrolled value." }
  }
}

Node %WrapSafeRate
{
  string %Category { "Utils" }
  string %Color { "Green" }
  string %Docs { "The shader time values wrap around every 1000 seconds to prevent floating point precision issues.\nRounds a rate (units per second) so that any periodic function of 'Time * Rate' with the given period does not jump when the time wraps around.\nUse a period of 1 for e.g. texture scrolling in UV space or 'frac', and 'Wrap Safe Angular Frequency' for sine and cosine." }
  string %Title { "Wrap Safe Rate: {$in0}" }

  InputPin %Rate
  {
    string %Type { "auto" }
    bool %Expose { true }
    string %DefaultValue { "1" }
    string %Tooltip { "The rate in units per second, e.g. a scroll speed." }
  }

  InputPin %Period
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "1" }
    string %Tooltip { "The period after which the function that uses the rate repeats, e.g. 1 for texture scrolling in UV space with a repeating sampler." }
  }

  OutputPin %Result
  {
    string %Type { "auto" }
    string %Inline { "WrapSafeRate($in0, ToFloat1($in1))" }
    string %Tooltip { "The rate rounded to a multiple of 'Period / 1000' (component-wise)." }
  }
}

Node %WrapSafeAngularFrequency
{
  string %Category { "Utils" }
  string %Color { "Green" }
  string %Docs { "The shader time values wrap around every 1000 seconds to prevent floating point precision issues.\nRounds an angular frequency (radians per second, as in 'sin(Time * Frequency)') so that the sine or cosine does not jump when the time wraps around." }
  string %Title { "Wrap Safe Angular Frequency: {$in0}" }

  InputPin %Frequency
  {
    string %Type { "auto" }
    bool %Expose { true }
    string %DefaultValue { "1" }
    string %Tooltip { "The angular frequency in radians per second." }
  }

  OutputPin %Result
  {
    string %Type { "auto" }
    string %Inline { "WrapSafeAngularFrequency($in0)" }
    string %Tooltip { "The angular frequency rounded to a multiple of '2 * PI / 1000' (component-wise)." }
  }
}

Node %SceneColor
{
  string %Category { "Utils" }
  string %Color { "Green" }
  string %Docs { "Samples and outputs the color of the rendered scene at a specified screen-space position.\nUseful for effects that require access to the underlying scene color, such as post-processing or custom blending." }

  InputPin %ScreenCoord
  {
    string %Type { "float2" }
    string %DefaultValue { "G.Input.Position.xy" }
    string %Tooltip { "Screen coordinate (X, Y) to sample the scene color from." }
  }

  OutputPin %Color
  {
    string %Type { "float3" }
    unsigned_int8 %Color { 200, 200, 200 }
    string %Inline { "SampleSceneColor(ToFloat2($in0))" }
    string %Tooltip { "The color sampled from the scene at the given screen coordinate." }
  }
}

Node %SceneDepth
{
  string %Category { "Utils" }
  string %Color { "Green" }
  string %Docs { "Samples and outputs the depth value from the scene at a specified screen coordinate.\nThis can be used for depth-based effects, such as fog, fading, or custom depth comparisons." }

  InputPin %ScreenCoord
  {
    string %Type { "float2" }
    string %DefaultValue { "G.Input.Position.xy" }
    string %Tooltip { "Screen coordinate (X, Y) to sample the scene depth from." }
  }

  OutputPin %Depth
  {
    string %Type { "float3" }
    string %Inline { "SampleSceneDepth(ToFloat2($in0))" }
    string %Tooltip { "The depth value sampled from the scene at the given screen coordinate." }
  }
}

Node %ScenePosition
{
  string %Category { "Utils" }
  string %Color { "Green" }
  string %Docs { "Outputs the world-space position corresponding to a given screen coordinate. Useful for reconstructing 3D positions from screen coordinates, enabling advanced effects like screen-space reflections or custom geometry manipulation." }

  InputPin %ScreenCoord
  {
    string %Type { "float2" }
    string %DefaultValue { "G.Input.Position.xy" }
    string %Tooltip { "Screen coordinate (X, Y) to sample the world position from." }
  }

  OutputPin %Position
  {
    string %Type { "float3" }
    string %Color { "Indigo" }
    string %Inline { "SampleScenePosition(ToFloat2($in0))" }
    string %Tooltip { "The world-space position at the given screen coordinate position." }
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

Node %PixelDepth
{
  string %Category { "Utils" }
  string %Color { "Green" }
  string %Docs { "Returns the depth at the current pixel." }
 
  OutputPin %Depth
  {
    string %Type { "float" }
    string %Inline { "G.Input.Position.w" }
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

float4 VisualShaderRefraction(float3 worldNormal, float distortionAmount, float newOpacity)
{
  float2 distortion;
  distortion.x = dot(worldNormal, G.Input.Tangent) * distortionAmount;
  distortion.y = dot(worldNormal, G.Input.BiTangent) * distortionAmount;

  float2 dummy;
  return CalculateRefraction(G.Input.WorldPosition, G.Input.Position, worldNormal, distortion, newOpacity, dummy);
}

float2 VisualShaderRefractedScreenCoord(float3 worldNormal, float distortionAmount)
{
  float2 distortion;
  distortion.x = dot(worldNormal, G.Input.Tangent) * distortionAmount;
  distortion.y = dot(worldNormal, G.Input.BiTangent) * distortionAmount;

  float2 coord;
  CalculateRefraction(G.Input.WorldPosition, G.Input.Position, worldNormal, distortion, 1, coord);
  return coord;
}

" }

  InputPin %Normal
  {
    string %Color { "Violet" }
    string %Type { "float3" }
    string %DefaultValue { "normalize(GetNormal())" }
    string %Tooltip { "Surface normal used to calculate refraction direction." }
  }
  
  InputPin %Distortion
  {
    string %Type { "float" }
    bool %Expose { true }
    string %DefaultValue { "0.1f" }
    string %Tooltip { "The amount of distortion." }
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
    string %Inline { "VisualShaderRefraction(ToFloat3($in0), ToFloat1($in1), ToFloat1($in2))" }
    string %Tooltip { "Resulting color and opacity after applying refraction, simulating light bending through the material." }
  }

  OutputPin %RefractedScreenCoord
  {
    string %Type { "float2" }
    string %Inline { "VisualShaderRefractedScreenCoord(ToFloat3($in0), ToFloat1($in1))" }
    string %Tooltip { "Screen coordinate after the refraction has been applied." }
  }
}
