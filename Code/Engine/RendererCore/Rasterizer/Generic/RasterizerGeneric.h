#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/SimdMath/SimdMat4f.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <RendererCore/RendererCoreDLL.h>

struct OccluderGeneric;

/// Cross-platform software rasterizer for occlusion culling.
///
/// Uses ezSimdVec4f / ezSimdVec4i (128-bit, 4-wide) instead of raw AVX2 intrinsics.
/// Processes 4 primitives per batch (vs 8 in the AVX2 path) and stores depth as 32-bit float
/// (vs 16-bit packed in the AVX2 path), trading some throughput/memory for full portability.
class EZ_RENDERERCORE_DLL RasterizerGeneric
{
public:
  RasterizerGeneric(ezUInt32 uiWidth, ezUInt32 uiHeight);

  void SetModelViewProjection(const float* pMatrix);
  void Clear();

  template <bool PossiblyNearClipped>
  void Rasterize(const OccluderGeneric& occluder);

  bool QueryVisibility(const ezSimdVec4f& vBoundsMin, const ezSimdVec4f& vBoundsMax, bool& out_bNeedsClipping);

  bool Query2D(ezUInt32 uiMinX, ezUInt32 uiMaxX, ezUInt32 uiMinY, ezUInt32 uiMaxY, float fMaxZ) const;

  void ReadBackDepth(void* pTarget) const;

private:
  template <bool PossiblyNearClipped>
  static void NormalizeEdge(ezSimdVec4f& inout_vNx, ezSimdVec4f& inout_vNy, const ezSimdVec4f& vEdgeFlipMask);

  static ezSimdVec4i QuantizeSlopeLookup(const ezSimdVec4f& vNx, const ezSimdVec4f& vNy);

  /// Version for rasterization context where edge normals have been pre-scaled by NormalizeEdge.
  static ezSimdVec4i QuantizeSlopeLookupScaled(const ezSimdVec4f& vNx, const ezSimdVec4f& vNy);

  static ezUInt32 QuantizeOffsetLookup(float fOffset);

  static void PrecomputeRasterizationTable();

  ezSimdMat4f m_MVP;

  static ezDynamicArray<ezInt64> s_PrecomputedRasterTables;

  /// 32-bit float depth per pixel. 64 floats per 8x8 block, stored in row-major order.
  /// Block layout: 8 rows of 8 floats.
  ezDynamicArray<float> m_DepthBuffer;

  /// One float per block: the minimum (closest) depth value in the block.
  /// A value of 0.0f indicates a cleared (empty) block.
  ezDynamicArray<float> m_HiZ;

  ezUInt32 m_uiWidth;
  ezUInt32 m_uiHeight;
  ezUInt32 m_uiBlocksX;
  ezUInt32 m_uiBlocksY;

  ezUInt64 s_BBB = 0;
};
