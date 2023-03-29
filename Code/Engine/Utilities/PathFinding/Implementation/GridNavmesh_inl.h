#pragma once

template <class CellData>
void ezGridNavmesh::CreateFromGrid(
  const ezGameGrid<CellData>& grid, CellComparator isSameCellType, void* pPassThrough, CellBlocked isCellBlocked, void* pPassThrough2)
{
  m_NodesGrid.CreateGrid(grid.GetGridSizeX(), grid.GetGridSizeY());

  UpdateRegion(ezRectU32(grid.GetGridSizeX(), grid.GetGridSizeY()), isSameCellType, pPassThrough, isCellBlocked, pPassThrough2);

  CreateGraphEdges();
}
