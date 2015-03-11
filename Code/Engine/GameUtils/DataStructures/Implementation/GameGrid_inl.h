#pragma once

template<class CellData>
ezGameGrid<CellData>::ezGameGrid()
{
  m_uiWidth = 0;
  m_uiHeight = 0;

  m_RotateToWorldspace.SetIdentity();
  m_RotateToGridspace.SetIdentity();

  m_vWorldSpaceOrigin.SetZero();
  m_vWorldSpaceCellSize.Set(1.0f);
  m_vInverseWorldSpaceCellSize.Set(1.0f);
}

template<class CellData>
void ezGameGrid<CellData>::CreateGrid(ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  m_Cells.Clear();

  m_uiWidth = uiWidth;
  m_uiHeight = uiHeight;

  m_Cells.SetCount(m_uiWidth * m_uiHeight);
}

template<class CellData>
void ezGameGrid<CellData>::SetWorldSpaceDimensions(const ezVec3& vLowerLeftCorner, const ezVec3& vCellSize, Orientation ori)
{
  ezMat3 mRot;

  switch (ori)
  {
  case InPlaneXY:
    mRot.SetIdentity();
    break;
  case InPlaneXZ:
    mRot.SetRotationMatrix(ezVec3(1, 0, 0), ezAngle::Degree(90.0f));
    break;
  case InPlaneXminusZ:
    mRot.SetRotationMatrix(ezVec3(1, 0, 0), ezAngle::Degree(-90.0f));
    break;
  }

  SetWorldSpaceDimensions(vLowerLeftCorner, vCellSize, mRot);
}

template<class CellData>
void ezGameGrid<CellData>::SetWorldSpaceDimensions(const ezVec3& vLowerLeftCorner, const ezVec3& vCellSize, const ezMat3& mRotation)
{
  m_vWorldSpaceOrigin  = vLowerLeftCorner;
  m_vWorldSpaceCellSize = vCellSize;
  m_vInverseWorldSpaceCellSize = ezVec3(1.0f).CompDiv(vCellSize);

  m_RotateToWorldspace = mRotation;
  m_RotateToGridspace  = mRotation.GetInverse();
}

template<class CellData>
ezVec2I32 ezGameGrid<CellData>::GetCellAtWorldPosition(const ezVec3& vWorldSpacePos) const
{
  const ezVec3 vCell = m_RotateToGridspace * ((vWorldSpacePos - m_vWorldSpaceOrigin).CompMult(m_vInverseWorldSpaceCellSize));

  // Without the Floor, the border case when the position is outside (-1 / -1) is not immediately detected
  return ezVec2I32((ezInt32) ezMath::Floor(vCell.x), (ezInt32) ezMath::Floor(vCell.y));
}

template<class CellData>
ezVec3 ezGameGrid<CellData>::GetCellWorldSpaceOrigin(const ezVec2I32& Coord) const
{
  const ezVec3 vPos = m_RotateToWorldspace * ezVec3((float) Coord.x, (float) Coord.y, 0.0f);

  return m_vWorldSpaceOrigin + m_vWorldSpaceCellSize.CompMult(vPos);
}

template<class CellData>
ezVec3 ezGameGrid<CellData>::GetCellWorldSpaceCenter(const ezVec2I32& Coord) const
{
  return GetCellWorldSpaceOrigin(Coord) + m_vWorldSpaceCellSize * 0.5f;
}


template<class CellData>
bool ezGameGrid<CellData>::IsValidCellCoordinate(const ezVec2I32& Coord) const
{
  return (Coord.x >= 0 && Coord.x < m_uiWidth &&
          Coord.y >= 0 && Coord.y < m_uiHeight);
}

template<class CellData>
bool ezGameGrid<CellData>::PickCell(const ezVec3& vRayStartPos, const ezVec3& vRayDirNorm, ezVec2I32* out_CellCoord, ezVec3* out_vIntersection) const
{
  ezPlane p;
  p.SetFromNormalAndPoint(m_RotateToWorldspace * ezVec3(0, 0, -1), m_vWorldSpaceOrigin);

  ezVec3 vPos;

  if (!p.GetRayIntersection(vRayStartPos, vRayDirNorm, nullptr, &vPos))
    return false;

  if (out_vIntersection)
    *out_vIntersection = vPos;

  if (out_CellCoord)
    *out_CellCoord = GetCellAtWorldPosition(vPos);

  return true;
}

template<class CellData>
ezBoundingBox ezGameGrid<CellData>::GetWorldBoundingBox() const
{
  ezVec3 vGridBox(m_uiWidth, m_uiHeight, 1.0f);

  vGridBox = m_RotateToWorldspace * vGridBox;

  return ezBoundingBox(m_vWorldSpaceOrigin, m_vWorldSpaceOrigin + m_vWorldSpaceCellSize.CompMult(vGridBox));
}

