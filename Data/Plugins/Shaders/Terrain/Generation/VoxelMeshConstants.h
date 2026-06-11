#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>
#include <Shaders/Common/Platforms.h>

/// Vertex written by the surface nets shaders into the mesh vertex buffer.
/// Material == 0xFFFFFFFF indicates an inactive (no edge crossing) cell slot.
struct EZ_SHADER_STRUCT VoxelGpuVertex
{
  FLOAT3(Position);
  UINT1(Material);          ///< Dominant material index (0-15). Value 0xFFFFFFFF = inactive cell sentinel.

  FLOAT3(Normal);
  FLOAT1(MaterialStrength); ///< Blend weight for the per-vertex override material (0 = main material, 1 = full override).

  FLOAT3(Tangent);
  FLOAT1(BitangentSign);
};

/// Atomic counters output by the surface nets shaders.
/// Padded to 16 bytes so sizeof matches alignas(16) EZ_SHADER_STRUCT in C++.
struct EZ_SHADER_STRUCT VoxelMeshCounts
{
  UINT1(PrimitiveCount); ///< Triangle count (2 per surface quad), written atomically by Pass 2.
  UINT1(VertexCount);    ///< Compact active vertex count, written atomically by Pass 1.
  UINT1(Pad1);
  UINT1(Pad2);
};
