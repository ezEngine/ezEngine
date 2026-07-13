#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>
#include <Shaders/Common/Platforms.h>

/// Push constants for HeightfieldTerrainBakeStep1CS and HeightfieldTerrainBakeStep2CS.
CONSTANT_BUFFER2(HeightfieldBakeConstants, 3, BG_DRAW_CALL)
{
  FLOAT1(GridSpacing);         ///< World-space distance between adjacent vertices.
  UINT1(VertexIdxPitch);       ///< Number of buffer entries per row (= full stored-grid width, including the 4-vertex border on each side).
  UINT1(BrushCount);           ///< Number of valid entries in the Brushes structured buffer. 0 means no brushes — heights come from SourceHeights only.
  UINT1(DefaultMaterialIndex); ///< Step2 only: material index used to fill any weight not covered by brushes during normalization.
  FLOAT2(PatchOrigin);         ///< World-space patch origin. Used to sample brush noise in absolute world space.
  UINT1(CellsPerSide);         ///< Step3 only: number of rendered quads per side (= VertexIdxPitch - 9).
};
