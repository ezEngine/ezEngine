#pragma once

template<class CellData>
void ezGameGrid<CellData>::CreateGrid(ezUInt16 uiWidth, ezUInt16 uiDepth, ezUInt8 uiSlices)
{
  m_Cells.Clear();

  m_uiWidth = uiWidth;
  m_uiDepth = uiDepth;
  m_uiSlices = uiSlices;

  m_Cells.SetCount(m_uiWidth * m_uiDepth * uiSlices);
}

template<class CellData>
void ezGameGrid<CellData>::SetWorldDimensions(const ezVec3& vLowerLeftCorner, const ezVec3& vCellSize)
{
  m_vWorldSpaceOrigin  = vLowerLeftCorner;
  m_vWorldSpaceCellSize = vCellSize;
  m_vInverseWorldSpaceCellSize = ezVec3(1.0f).CompDiv(vCellSize);
}

template<class CellData>
ezUInt32 ezGameGrid<CellData>::GetCellIndex(const ezGridCoordinate& Coord) const
{
  return (Coord.Slice * m_uiDepth * m_uiWidth) + (Coord.z * m_uiWidth) + Coord.x;
}

template<class CellData>
ezUInt32 ezGameGrid<CellData>::GetNumCells() const
{
  return m_uiWidth * m_uiDepth * m_uiSlices;
}

template<class CellData>
ezGridCoordinate ezGameGrid<CellData>::GetCellCoordsByInex(ezUInt32 uiIndex) const
{
  const ezUInt32 uiCellsPerSlice = m_uiWidth * m_uiDepth;

  ezGridCoordinate c;

  c.Slice = uiIndex / uiCellsPerSlice;
  uiIndex -= c.Slice * uiCellsPerSlice;

  c.z = uiIndex / m_uiWidth;
  c.x = uiIndex % m_uiWidth;
  
  return c;
}

template<class CellData>
CellData& ezGameGrid<CellData>::GetCellByIndex(ezUInt32 uiIndex)
{
  return m_Cells[uiIndex];
}

template<class CellData>
const CellData& ezGameGrid<CellData>::GetCellByIndex(ezUInt32 uiIndex) const
{
  return m_Cells[uiIndex];
}

template<class CellData>
CellData& ezGameGrid<CellData>::GetCell(const ezGridCoordinate& Coord)
{
  return m_Cells[GetCellIndex(Coord)];
}

template<class CellData>
const CellData& ezGameGrid<CellData>::GetCell(const ezGridCoordinate& Coord) const
{
  return m_Cells[GetCellIndex(Coord)];
}

template<class CellData>
ezGridCoordinate ezGameGrid<CellData>::GetCellAtPosition(const ezVec3& vWorldSpacePos) const
{
  const ezVec3 vCell = (vWorldSpacePos - m_vWorldSpaceOrigin).CompMult(m_vInverseWorldSpaceCellSize);

  return ezGridCoordinate((ezInt32) vCell.x, (ezInt32) vCell.z, (ezInt32) vCell.y);
}

template<class CellData>
ezVec3 ezGameGrid<CellData>::GetCellOrigin(const ezGridCoordinate& Coord) const
{
  return m_vWorldSpaceOrigin + ezVec3((float) Coord.x * m_vWorldSpaceCellSize.x, (float) Coord.Slice * m_vWorldSpaceCellSize.y, (float) Coord.z * m_vWorldSpaceCellSize.z);
}

template<class CellData>
ezVec3 ezGameGrid<CellData>::GetCellSize() const
{
  return m_vWorldSpaceCellSize;
}

template<class CellData>
bool ezGameGrid<CellData>::IsValidCellCoordinate(const ezGridCoordinate& Coord) const
{
  return (Coord.x     >= 0 && Coord.x     < m_uiWidth &&
          Coord.z     >= 0 && Coord.z     < m_uiDepth &&
          Coord.Slice >= 0 && Coord.Slice < m_uiSlices);
}

template<class CellData>
ezGridCoordinate ezGameGrid<CellData>::PickCell(const ezVec3& vRayStartPos, const ezVec3& vRayDirNorm, ezVec3* out_vIntersection, ezUInt8 uiSlice) const
{
  ezPlane p;
  p.SetFromNormalAndPoint(ezVec3(0, 1, 0), m_vWorldSpaceOrigin + m_vWorldSpaceCellSize * (float) uiSlice);

  ezVec3 vPos;

  if (!p.GetRayIntersection(vRayStartPos, vRayDirNorm, NULL, &vPos))
    return ezGridCoordinate(-1, -1, -1);

  if (out_vIntersection)
    *out_vIntersection = vPos;

  return GetCellAtPosition(vPos);
}

template<class CellData>
ezBoundingBox ezGameGrid<CellData>::GetBoundingBox() const
{
  return ezBoundingBox(m_vWorldSpaceOrigin, m_vWorldSpaceOrigin + m_vWorldSpaceCellSize.CompMult(ezVec3(m_uiWidth, m_uiSlices, m_uiDepth)));
}

