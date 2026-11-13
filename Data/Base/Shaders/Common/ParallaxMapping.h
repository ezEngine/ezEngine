#pragma once

float PomDepthFixedPoint(Texture2D heightTex, SamplerState samp, float2 texCoords, float2 delta, float relax)
{
  float t = 0.0;
  // Hard-coded value 10, because this value should be neither too small nor too large
  // Small number of iterations may lead to poor accuracy
  // But large number of iterations may lead to drastically bad results in case of divergence
  const int iterations = 10;
  // Relaxed fixed-point iteration: t = depth(texCoords - t * delta)
  [unroll] for (int i = 0; i < iterations; ++i)
  {
    float d = heightTex.SampleLevel(samp, texCoords - t * delta, 0).r;
    t = lerp(t, d, relax);
  }

  return t;
}

/// Performs parallax occlusion mapping using steep parallax with binary search refinement.
float PomDepthLinear(Texture2D heightTex, SamplerState samp, float2 texCoords, float2 delta, int numSteps)
{
  float layerDepth = 1.0 / (float)numSteps;
  float currentLayerDepth = 0.0;

  // UV offset per step
  float2 deltaUV = delta / (float)numSteps;

  float2 currentUV = texCoords;
  float currentHeight = heightTex.SampleLevel(samp, currentUV, 0).r;

  // Steep parallax: march until we go below the surface
  [loop] for (int i = 0; i < numSteps && currentLayerDepth < currentHeight; ++i)
  {
    currentUV -= deltaUV;
    currentHeight = heightTex.SampleLevel(samp, currentUV, 0).r;
    currentLayerDepth += layerDepth;
  }

  // Binary search refinement between the last two steps for sub-step precision
  float2 prevUV = currentUV + deltaUV;
  float prevLayerDepth = currentLayerDepth - layerDepth;

  [unroll] for (int j = 0; j < 4; ++j)
  {
    float2 midUV = (prevUV + currentUV) * 0.5;
    float midLayerDepth = (prevLayerDepth + currentLayerDepth) * 0.5;
    float midHeight = heightTex.SampleLevel(samp, midUV, 0).r;

    if (midHeight > midLayerDepth)
    {
      // Still above surface, step further in
      prevUV = midUV;
      prevLayerDepth = midLayerDepth;
    }
    else
    {
      // Below surface, step back
      currentUV = midUV;
      currentLayerDepth = midLayerDepth;
    }
  }

  return currentLayerDepth;
}

float2 PomDelta(float3 viewDirTS, float heightScale)
{
  // Full UV offset at depth 1, scaled by height and projected by view angle
  return (viewDirTS.xy / max(viewDirTS.z, 0.001)) * heightScale;
}

/// Performs parallax occlusion mapping using fixed point iterations to resolve depth value.
/// The height map convention is white (1.0) = deepest point, black (0.0) = surface.
/// viewDirTS must be normalized. heightScale controls maximum displacement depth in UV space.
/// relax controls fixed point relaxation, changing it may affect convergence and accuracy
float2 ParallaxOcclusionMappingFixedPoint(Texture2D heightTex, SamplerState samp, float2 texCoords, float3 viewDirTS, float heightScale, float relax, out float depth)
{
  float2 delta = PomDelta(viewDirTS, heightScale);
  depth = PomDepthFixedPoint(heightTex, samp, texCoords, delta, relax);
  return texCoords - depth * delta;
}

/// Performs parallax occlusion mapping using steep parallax with binary search refinement.
/// The height map convention is white (1.0) = deepest point, black (0.0) = surface.
/// viewDirTS must be normalized. heightScale controls maximum displacement depth in UV space.
/// maxSteps is an upper bound; actual step count is reduced at near-perpendicular view angles.
float2 ParallaxOcclusionMappingLinearSearch(Texture2D heightTex, SamplerState samp, float2 texCoords, float3 viewDirTS, float heightScale, int maxSteps, out float depth)
{
  float2 delta = PomDelta(viewDirTS, heightScale);
  int numSteps = (int)lerp((float)maxSteps, (float)max(maxSteps / 4, 4), abs(viewDirTS.z));
  depth = PomDepthLinear(heightTex, samp, texCoords, delta, numSteps);
  return texCoords - depth * delta;
}

/// Performs parallax occlusion mapping using fixed point iterations moderate angles between view and surface
/// and steep parallax (linear search) with binary search refinement for steep ones.
/// The height map convention is white (1.0) = deepest point, black (0.0) = surface.
/// viewDirTS must be normalized. heightScale controls maximum displacement depth in UV space.
/// maxSteps is an upper bound for linear search; actual step count is reduced at near-perpendicular view angles.
/// relax controls fixed point relaxation, changing it may affect convergence and accuracy
/// if viewDirTS.z > linearBelow + 0.1 only the fixed point iterations search is used 
/// if viewDirTS.z < linearBelow only the linear search with binary refinement is used
/// otherwise the results of both methods are linearly interpolated
float2 ParallaxOcclusionMappingHybrid(Texture2D heightTex, SamplerState samp, float2 texCoords, float3 viewDirTS, float heightScale, int maxSteps, float relax, float linearBelow, out float depth)
{
  // linearBelow should be strictly less than 1.0 otherwise the denominator in "w" may blow up
  const float fixedPointAbove = saturate(linearBelow + 0.1); // viewDirTS.z above this: fixed point only
  float2 delta = PomDelta(viewDirTS, heightScale);
  // Blend weight: 0 = fixed point, 1 = linear search
  float w = saturate((fixedPointAbove - viewDirTS.z) / (fixedPointAbove - linearBelow));

  float tFP = 0.0;
  float tLS = 0.0;

  [branch] if (w < 1.0)
  {
    tFP = PomDepthFixedPoint(heightTex, samp, texCoords, delta, relax);
  }

  [branch] if (w > 0.0)
  {
    int numSteps = (int)lerp((float)maxSteps, (float)max(maxSteps / 4, 4), abs(viewDirTS.z));
    tLS = PomDepthLinear(heightTex, samp, texCoords, delta, numSteps);
  }

  depth = lerp(tFP, tLS, w);
  return texCoords - depth * delta;
}
