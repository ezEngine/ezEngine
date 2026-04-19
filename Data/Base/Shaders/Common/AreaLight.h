#pragma once

/// Projects 'queryPoint' onto the line segment from 'segmentStart' to 'segmentEnd'; returns the closest point on the segment.
float3 ClosestPointOnSegment(float3 queryPoint, float3 segmentStart, float3 segmentEnd)
{
  float3 segment = segmentEnd - segmentStart;
  float t = saturate(dot(queryPoint - segmentStart, segment) / dot(segment, segment));
  return segmentStart + t * segment;
}

/// Computes the most representative point on a capsule (segment from 'segmentStart' to 'segmentEnd' with the given 'tubeRadius') for a specular reflection ray 'reflectionDir' originating from 'shadingPosition'. Returns the unnormalized vector from 'shadingPosition' to that point; the caller can use its length both to normalize the direction and to derive roughness widening / attenuation terms.
///
/// First finds the point on the segment closest to the ray (Picott 1992), then offsets it toward the ray by the tube radius Karis 2013, sphere representative point).
float3 ClosestPointOnSegmentToRay(float3 shadingPosition, float3 segmentStart, float3 segmentEnd, float tubeRadius, float3 reflectionDir)
{
  float3 toStart = segmentStart - shadingPosition;
  float3 toEnd = segmentEnd - shadingPosition;
  float3 segment = toEnd - toStart;
  float reflDotSegment = dot(reflectionDir, segment);
  float t = dot(reflectionDir, toStart) * reflDotSegment - dot(toStart, segment);
  t /= dot(segment, segment) - reflDotSegment * reflDotSegment;
  float3 closestOnAxis = toStart + saturate(t) * segment;

  if (tubeRadius > 0)
  {
    float3 centerToRay = mad(dot(closestOnAxis, reflectionDir), reflectionDir, -closestOnAxis);
    closestOnAxis = mad(centerToRay, saturate(tubeRadius / length(centerToRay)), closestOnAxis);
  }

  return closestOnAxis;
}

/// Representative point on a directional emitter disc (e.g. the sun disc) for a specular reflection ray 'reflectionDir'. The disc has angular half-size 'halfAngle', given as sin(halfAngle) to match the packed value used for directional lights. 'lightDir' is the unit direction toward the emitter.
///
/// When 'reflectionDir' already lies inside the cone subtended by the disc (dot(reflectionDir, lightDir) >= cos(halfAngle)), it is returned unchanged, giving a perfect mirror hit. Otherwise it is clamped to the disc rim by projecting onto the cone (Karis 2013, MRP specialised to an infinitely distant disc). The result is a unit vector.
float3 ClosestPointOnDirectionalDisc(float3 lightDir, float3 reflectionDir, float sinHalfAngle)
{
  float cosHalfAngle = sqrt(saturate(1.0 - sinHalfAngle * sinHalfAngle));
  float reflDotLight = dot(lightDir, reflectionDir);
  if (reflDotLight >= cosHalfAngle)
  {
    return reflectionDir;
  }

  float3 tangent = reflectionDir - reflDotLight * lightDir;
  tangent *= rsqrt(max(dot(tangent, tangent), 1e-12));
  return lightDir * cosHalfAngle + tangent * sinHalfAngle;
}
