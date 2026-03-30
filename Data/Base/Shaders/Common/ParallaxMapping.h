#pragma once

/// Performs parallax occlusion mapping using steep parallax with binary search refinement.
///
/// The height map convention is white (1.0) = surface, black (0.0) = deepest point.
/// viewDirTS must be normalized. heightScale controls maximum displacement depth in UV space.
/// maxSteps is an upper bound; actual step count is reduced at near-perpendicular view angles.
float2 ParallaxOcclusionMapping(Texture2D heightTex, SamplerState samp, float2 texCoords, float3 viewDirTS, float heightScale, int maxSteps)
{
  // Fewer steps when looking straight down (viewDirTS.z close to 1), more at grazing angles
  int numSteps = (int)lerp((float)maxSteps, (float)max(maxSteps / 4, 4), abs(viewDirTS.z));

  float layerDepth = 1.0 / (float)numSteps;
  float currentLayerDepth = 0.0;

  // UV offset per step, scaled by height and projected by view angle
  float2 deltaUV = (viewDirTS.xy / max(viewDirTS.z, 0.001)) * heightScale / (float)numSteps;

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

  return currentUV;
}
