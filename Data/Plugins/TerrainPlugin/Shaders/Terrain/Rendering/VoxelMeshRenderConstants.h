#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>
#include <Shaders/Common/Platforms.h>

/// Push constants for the VoxelMeshMaterial vertex shader.
/// Provides the instance data slot for the world transform and picking ID.
/// Only include this in render shaders, NOT compute shaders, to avoid conflicts.
BEGIN_PUSH_CONSTANTS(VoxelMeshRenderConstants)
{
  UINT1(InstanceDataOffset);
  UINT1(BaseMaterialIndex); ///< Volume's configured base material; used as the "main" layer blended with the per-vertex override material via VoxelGpuVertex::MaterialStrength (0 = base only, 1 = full override).
}
END_PUSH_CONSTANTS(VoxelMeshRenderConstants)
