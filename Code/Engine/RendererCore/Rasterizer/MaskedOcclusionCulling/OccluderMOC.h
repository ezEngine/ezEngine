#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <RendererCore/RendererCoreDLL.h>

/// Occluder data stored in a format suitable for Intel's MaskedOcclusionCulling library.
///
/// Vertices are stored as (x, y, z, w) floats in object space with w = 1.
/// Triangles are defined by index triplets into the vertex array.
struct EZ_RENDERERCORE_DLL OccluderMOC
{
  /// Vertex positions as packed [x, y, z, w] floats. w is always 1.0.
  ezDynamicArray<float> m_Vertices;

  /// Triangle indices (3 per triangle) into the vertex array.
  ezDynamicArray<ezUInt32> m_Indices;

  ezUInt32 m_uiNumTriangles = 0;

  ezVec3 m_vBoundsMin = ezVec3(ezMath::MaxValue<float>());
  ezVec3 m_vBoundsMax = ezVec3(-ezMath::MaxValue<float>());

  void Bake(const ezDynamicArray<ezVec3>& vertices, const ezDynamicArray<ezUInt32>& triangleIndices);
};
