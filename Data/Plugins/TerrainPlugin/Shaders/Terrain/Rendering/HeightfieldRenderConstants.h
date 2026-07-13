#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>
#include <Shaders/Common/Platforms.h>

/// Push constants for the heightfield terrain vertex and pixel shaders.
BEGIN_PUSH_CONSTANTS(HeightfieldRenderConstants)
{
  FLOAT1(GridSpacing);         ///< World-space distance between adjacent full-resolution vertices.
  UINT1(FirstVertexIdx);       ///< Buffer index of the inner render grid's top-left corner (skips the 4-vertex border).
  UINT1(VertexIdxPitch);       ///< Number of buffer entries per row.
  UINT1(CellsPerSide);         ///< Number of full-resolution quads per side. Used to map rendered cells to baked material/weight cells.
  UINT1(InstanceDataOffset);   ///< Offset into the perInstanceData buffer for this patch (used for editor picking and world transform).
  UINT1(FallbackMaterialSlot); ///< Material slot index (0–7) used as the implicit fallback layer. Weight = max(0, 1 - w0 - w1 - w2 - w3).
  UINT1(RenderCellsPerSide);   ///< Number of rendered quads per side, including skirt rings. Drives cellIndex decode.
  UINT1(VertexStep);           ///< Stored-buffer index step between adjacent rendered vertices (2^LOD).
  UINT1(SkirtCells);           ///< Number of skirt cell rings rendered on each side (0 = no skirt).
  FLOAT1(SkirtDepth);          ///< World-space downward offset applied to skirt vertices.
  FLOAT1(LodFade);             ///< [0, 1] blend of the vertices that vanish at the next-coarser LOD toward their interpolated position.
}
END_PUSH_CONSTANTS(HeightfieldRenderConstants)
