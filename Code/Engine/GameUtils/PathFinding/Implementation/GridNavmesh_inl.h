#pragma once

template<class CellData>
void ezGridNavmesh::CreateFromGrid(const ezGameGrid<CellData>& Grid, CellComparator IsSameCellType, void* pPassThrough, CellBlocked IsCellBlocked, void* pPassThrough2)
{
  m_NodesGrid.CreateGrid(Grid.GetGridWidth(), Grid.GetGridHeight());

  UpdateRegion(ezRectU32(Grid.GetGridWidth(), Grid.GetGridHeight()), IsSameCellType, pPassThrough, IsCellBlocked, pPassThrough2);

  CreateGraphEdges();
}

