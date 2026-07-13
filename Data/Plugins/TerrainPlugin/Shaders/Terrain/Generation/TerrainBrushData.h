#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>
#include <Shaders/Common/Platforms.h>

/// Brush data uploaded to the GPU once per bake and indexed by BrushCount in the push constants.
/// Shared between heightfield and voxel generation shaders.
struct EZ_SHADER_STRUCT TerrainBrushData
{
  FLOAT3(Position);   ///< Local-space position relative to the terrain patch origin.
  PACKEDHALF2(HalfExtentX, HalfExtentZ, PackedExtentsXZ);

  FLOAT3(InvRotRow0); ///< Rows 0-2 together form the 3x3 inverse rotation matrix of the brush.
  PACKEDHALF2(InnerRadius, OuterRadius, PackedRadii);

  FLOAT3(InvRotRow1);
  PACKEDHALF2(MaterialStrength, Falloff, PackedMatStrFalloff);

  FLOAT3(InvRotRow2);
  PACKEDUINT8(ModifyMode, MaterialIndex, Unused0, Unused1, PackedModeMat);

  PACKEDHALF2(NoiseStrength, NoiseFrequency, PackedNoise);
  PACKEDHALF2(HalfExtentYBottom, HalfExtentYTop, PackedExtentsY); ///< Trapezoid half-extents: bottom and top widths along the local Y axis.

  FLOAT1(CpuPriority);                                            ///< CPU-side sort key only; not read by shaders.
  FLOAT1(Padding0);
};

#define ezTerrainModifyMode_Max 0         ///< 2D: raise terrain up to brush height, never lower
#define ezTerrainModifyMode_Min 1         ///< 2D: lower terrain down to brush height, never raise
#define ezTerrainModifyMode_Set 2         ///< 2D: set terrain to brush height (raises and lowers)
#define ezTerrainModifyMode_Carve 3       ///< 3D: subtract brush volume from the terrain (hollow out)
#define ezTerrainModifyMode_Add 4         ///< 3D: add brush volume to the terrain (fill in)
#define ezTerrainModifyMode_OnlyPaint2D 5 ///< 2D footprint, no height change — material painting only
#define ezTerrainModifyMode_OnlyPaint3D 6 ///< 3D volume, no shape change — material painting only
