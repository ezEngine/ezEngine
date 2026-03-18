#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <RendererCore/RendererCoreDLL.h>

struct EZ_RENDERERCORE_DLL OccluderGeneric
{
  void Bake(const ezSimdVec4f* pVertices, ezUInt32 uiNumVertices, const ezSimdVec4f& vRefMin, const ezSimdVec4f& vRefMax);

  ezSimdVec4f m_vCenter;
  ezSimdVec4f m_vRefMin;
  ezSimdVec4f m_vRefMax;
  ezSimdVec4f m_vBoundsMin;
  ezSimdVec4f m_vBoundsMax;

  /// Packed vertex data in 11/11/10 format. Groups of 4 uint32 values (one per primitive vertex).
  /// Layout: for every 4 quads, there are 16 uint32 values organized as:
  ///   [V0_quad0, V0_quad1, V0_quad2, V0_quad3,  <- packet 0
  ///    V1_quad0, V1_quad1, V1_quad2, V1_quad3,  <- packet 1
  ///    V2_quad0, V2_quad1, V2_quad2, V2_quad3,  <- packet 2
  ///    V3_quad0, V3_quad1, V3_quad2, V3_quad3]  <- packet 3
  ezDynamicArray<ezUInt32> m_VertexData;
  ezUInt32 m_uiPacketCount = 0;
};
