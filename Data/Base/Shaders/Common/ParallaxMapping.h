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
  float currentDepth = 1.0 - heightTex.SampleLevel(samp, currentUV, 0).r;

  // Steep parallax: march until we go below the surface
  [loop] for (int i = 0; i < numSteps && currentLayerDepth < currentDepth; ++i)
  {
    currentUV -= deltaUV;
    currentDepth = 1.0 - heightTex.SampleLevel(samp, currentUV, 0).r;
    currentLayerDepth += layerDepth;
  }

  // Binary search refinement between the last two steps for sub-step precision
  float2 prevUV = currentUV + deltaUV;
  float prevLayerDepth = currentLayerDepth - layerDepth;

  [unroll] for (int j = 0; j < 4; ++j)
  {
    float2 midUV = (prevUV + currentUV) * 0.5;
    float midLayerDepth = (prevLayerDepth + currentLayerDepth) * 0.5;
    float midDepth = 1.0 - heightTex.SampleLevel(samp, midUV, 0).r;

    if (midDepth > midLayerDepth)
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

/// Performs parallax occlusion mapping with relaxed cone stepping.
///
/// The cone map stores the height in R (white = surface, black = deepest point) and the relaxed cone ratio in G.
/// The cone ratio is the horizontal distance per unit of depth, measured in units of the larger texture dimension.
/// Stepping is done in that normalized texel space, so non-square textures need no extra aspect ratio correction,
/// as long as the texture is mapped with the same texel density along U and V.
///
/// viewDirTS must be normalized. heightScale is the maximum displacement depth, in units of the larger texture dimension.
/// heightOffset moves the zero plane into the height map (0 = the brightest texels are at the surface, 0.5 = mid gray is at the surface).
/// maxSteps is the number of cone steps, which is followed by a fixed number of binary search steps.
float2 ParallaxConeStepMapping(Texture2D coneTex, SamplerState samp, float2 texCoords, float3 viewDirTS, float heightScale, float heightOffset, int maxSteps)
{
  float2 texSize;
  coneTex.GetDimensions(texSize.x, texSize.y);
  const float2 uvToNormalized = texSize / max(texSize.x, texSize.y);

  // ray direction per unit of depth, in normalized texel space and in UV space
  const float2 rayDirN = -(viewDirTS.xy / max(viewDirTS.z, 0.001)) * heightScale;
  const float rayRatio = length(rayDirN);
  const float3 rayDir = float3(rayDirN / uvToNormalized, 1.0);

  // mip selection based on the original texture coordinate, the shifted coordinates are continuous anyway
  const float2 dx = ddx(texCoords);
  const float2 dy = ddy(texCoords);

  // move the start of the ray backwards, such that the surface plane is at depth 'heightOffset'
  float3 pos = float3(texCoords - rayDir.xy * heightOffset, 0.0);
  float3 prevPos = pos;

  [loop] for (int i = 0; i < maxSteps; ++i)
  {
    const float2 cone = coneTex.SampleGrad(samp, pos.xy, dx, dy).rg;
    const float depthAbove = saturate((1.0 - cone.r) - pos.z);

    if (depthAbove <= 0.0)
      break;

    const float coneRatio = cone.g;
    const float d = (coneRatio * depthAbove) / (rayRatio + coneRatio);

    prevPos = pos;
    // the small bias prevents getting stuck at texels with a cone ratio of zero
    pos += rayDir * (d + 0.01);
  }

  // relaxed cones guarantee at most one intersection between the last two positions
  [unroll] for (int j = 0; j < 6; ++j)
  {
    const float3 mid = (prevPos + pos) * 0.5;
    const float midDepth = 1.0 - coneTex.SampleGrad(samp, mid.xy, dx, dy).r;

    if (mid.z < midDepth)
      prevPos = mid; // still above the surface
    else
      pos = mid;
  }

  return pos.xy;
}
