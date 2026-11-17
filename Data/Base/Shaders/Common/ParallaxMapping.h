#pragma once

/// Performs parallax occlusion mapping with relaxed cone stepping.
///
/// The cone map stores the height in R (white = surface, black = deepest point) and the relaxed cone ratio in G.
/// The cone ratio is the horizontal distance per unit of depth, measured in units of the larger texture dimension.
/// Stepping is done in that normalized texel space, so non-square textures need no extra aspect ratio correction,
/// as long as the texture is mapped with the same texel density along U and V.
///
/// viewDirTS must be normalized. heightScale is the maximum displacement depth, in units of the larger texture dimension.
/// heightOffset moves the zero plane into the height map (0 = the brightest texels are at the surface, 0.5 = mid gray is at the surface).
/// Returns the shifted texture coordinate. 'depth' receives the depth of the found intersection below the polygon, as a fraction of heightScale,
/// for ParallaxDepthOffset(). It is negative where the relief sticks out in front of the polygon due to heightOffset.
///
/// The function takes maxSteps + 6 texture samples in total.
/// Cone steps continue until the ray is below the surface, the remaining samples are used for a binary search, followed by a secant step.
/// Where the cones are narrow (at the foot of steep walls, or at grazing angles where the ray is much flatter than the cones),
/// each cone step only covers a fraction of the remaining depth, so those rays need more steps and get less refinement.
/// If the samples run out before the ray reaches the surface, the result is the last position above the surface.
float2 ParallaxConeStepMapping(Texture2D coneTex, SamplerState samp, float2 texCoords, float3 viewDirTS, float heightScale, float heightOffset, int maxSteps, out float depth)
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

  // ray depth minus surface depth, negative above the surface
  float prevDiff = -1.0;
  float diff = -1.0;

  const int maxSamples = maxSteps + 6;

  int i = 0;
  [loop] for (; i < maxSamples; ++i)
  {
    const float2 cone = coneTex.SampleGrad(samp, pos.xy, dx, dy).rg;
    diff = pos.z - (1.0 - cone.r);

    if (diff >= 0.0)
      break;

    const float coneRatio = cone.g;
    // both ratios are zero when looking straight down onto a texel with a cone ratio of zero (e.g. orthographic cameras)
    const float coneStep = (coneRatio * -diff) / max(rayRatio + coneRatio, 1e-6);

    prevPos = pos;
    prevDiff = diff;

    // the small bias prevents getting stuck at texels with a cone ratio of zero
    // at grazing angles it corresponds to several texels, larger values skip more thin features, smaller ones leave more rays above the surface
    pos += rayDir * (coneStep + 0.005);
  }

  // relaxed cones guarantee at most one intersection between the last two positions
  // with i == 0 the start position is already on or below the surface and there is nothing to refine
  [loop] for (int j = (i == 0) ? maxSamples : i + 1; j < maxSamples; ++j)
  {
    const float3 mid = (prevPos + pos) * 0.5;
    const float midDiff = mid.z - (1.0 - coneTex.SampleGrad(samp, mid.xy, dx, dy).r);

    if (midDiff < 0.0)
    {
      // still above the surface
      prevPos = mid;
      prevDiff = midDiff;
    }
    else
    {
      pos = mid;
      diff = midDiff;
    }
  }

  // secant step: treat the surface as planar between the two remaining positions
  // does nothing when the start position is already below the surface (prevPos == pos then)
  const float t = (diff >= 0.0 && prevDiff < 0.0) ? (-prevDiff / (diff - prevDiff)) : 1.0;
  pos = lerp(prevPos, pos, t);

  depth = pos.z - heightOffset;
  return pos.xy;
}

/// Computes how far the pixel has to be moved away from the camera, along the view ray, to lie on the found intersection.
///
/// depth and heightScale are the values used for and returned by ParallaxConeStepMapping(), coneTex is the same texture.
/// The world scale of the texture is estimated from screen space derivatives, so it is only correct for roughly uniform texel density.
/// The result is never negative: relief in front of the polygon (negative depth) stays at the polygon's depth,
/// because the depth may only be moved away from the camera (SV_DepthLessEqual).
float ParallaxDepthOffset(Texture2D coneTex, float depth, float heightScale, float2 texCoords, float3 worldPosition, float viewDirZ)
{
  // heightScale is measured in units of the larger texture dimension
  float2 texSize;
  coneTex.GetDimensions(texSize.x, texSize.y);
  const float2 texCoordsToUnits = texSize / max(texSize.x, texSize.y);

  const float2 dUnitsDx = ddx(texCoords) * texCoordsToUnits;
  const float2 dUnitsDy = ddy(texCoords) * texCoordsToUnits;
  const float worldPerUnit = (length(ddx(worldPosition)) + length(ddy(worldPosition))) / max(length(dUnitsDx) + length(dUnitsDy), 1e-6);

  // depth below the polygon in world units, converted to a distance along the view ray
  return max(0.0, depth * heightScale * worldPerUnit / max(viewDirZ, 0.001));
}
