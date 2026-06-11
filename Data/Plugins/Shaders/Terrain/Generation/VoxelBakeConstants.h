#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>
#include <Shaders/Common/Platforms.h>

/// Push constants shared by voxel generation compute shaders
CONSTANT_BUFFER2(VoxelBakeConstants, 3, BG_DRAW_CALL)
{
  FLOAT3(PatchOrigin);    ///< VoxelBakeCS only: world-space position of the volume origin. Used to sample noise in absolute world space.
  FLOAT1(GridSpacing);    ///< World-space size of one voxel.

  UINT3(VoxelResolution); ///< User-selected inner resolution per axis (no border, no alignment).
  UINT1(NumBorderVoxels); ///< Border voxels added on each side of the inner volume (typically 4).

  UINT3(BufferSize);      ///< Total voxel buffer dimensions: x aligned to multiple of 8, y/z = VoxelResolution.y/z + 2*NumBorderVoxels.
  UINT1(EdgeAxis);        ///< VoxelSurfaceNetsPass2CS only: which edge axis to process — 0 = X, 1 = Y, 2 = Z.

  UINT1(BrushCount);      ///< VoxelBakeCS only: number of valid entries in the Brushes structured buffer.
  UINT1(InitialSolid);    ///< VoxelBakeCS only: 0 = start as air, 1 = start as solid below FillHeight.
  FLOAT1(FillHeight);     ///< VoxelBakeCS only: local-space Z threshold for the initial solid/air split (only when InitialSolid != 0).
  FLOAT1(SmoothFactor);   ///< VoxelBlurDistCS only: blend weight between original and averaged distance (0 = passthrough, 1 = full 3³ average).
};
