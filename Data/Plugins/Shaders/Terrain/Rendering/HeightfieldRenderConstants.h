#pragma once

#include <Shaders/Common/ConstantBufferMacros.h>
#include <Shaders/Common/Platforms.h>

/// Push constants for the heightfield terrain vertex and pixel shaders.
BEGIN_PUSH_CONSTANTS(HeightfieldRenderConstants)
{
  FLOAT1(GridSpacing);         ///< World-space distance between adjacent vertices.
  UINT1(FirstVertexIdx);       ///< Buffer index of the first vertex to render (top-left corner of the inner render grid, skipping the 4-vertex border).
  UINT1(VertexIdxPitch);       ///< Number of buffer entries per row.
  UINT1(CellsPerSide);         ///< Number of rendered quads per side. Vertex count = CellsPerSide + 1.
  UINT1(InstanceDataOffset);   ///< Offset into the perInstanceData buffer for this patch (used for editor picking and world transform).
  UINT1(FallbackMaterialSlot); ///< Material slot index (0–7) used as the implicit fallback layer. Weight = max(0, 1 - w0 - w1 - w2 - w3).
}
END_PUSH_CONSTANTS(HeightfieldRenderConstants)
