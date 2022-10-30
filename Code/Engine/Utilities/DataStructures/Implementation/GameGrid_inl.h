#pragma once

template <class CellData>
ezGameGrid<CellData>::ezGameGrid()
{
  m_uiGridSizeX = 0;
  m_uiGridSizeY = 0;

  m_mRotateToWorldspace.SetIdentity();
  m_mRotateToGridspace.SetIdentity();

  m_vWorldSpaceOrigin.SetZero();
  m_vLocalSpaceCellSize.Set(1.0f);
  m_vInverseLocalSpaceCellSize.Set(1.0f);
}

template <class CellData>
void ezGameGrid<CellData>::CreateGrid(ezUInt16 uiSizeX, ezUInt16 uiSizeY)
{
  m_Cells.Clear();

  m_uiGridSizeX = uiSizeX;
  m_uiGridSizeY = uiSizeY;

  m_Cells.SetCount(m_uiGridSizeX * m_uiGridSizeY);
}

template <class CellData>
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

template <class CellData>
void ezGameGrid<CellData>::SetWorldSpaceDimensions(const ezVec3& vLowerLeftCorner, const ezVec3& vCellSize, const ezMat3& mRotation)
{
  m_vWorldSpaceOrigin = vLowerLeftCorner;
  m_vLocalSpaceCellSize = vCellSize;
  m_vInverseLocalSpaceCellSize = ezVec3(1.0f).CompDiv(vCellSize);

  m_mRotateToWorldspace = mRotation;
  m_mRotateToGridspace = mRotation.GetInverse();
}

template <class CellData>
ezVec2I32 ezGameGrid<CellData>::GetCellAtWorldPosition(const ezVec3& vWorldSpacePos) const
{
  const ezVec3 vCell = (m_mRotateToGridspace * ((vWorldSpacePos - m_vWorldSpaceOrigin)).CompMul(m_vInverseLocalSpaceCellSize));

  // Without the Floor, the border case when the position is outside (-1 / -1) is not immediately detected
  return ezVec2I32((ezInt32)ezMath::Floor(vCell.x), (ezInt32)ezMath::Floor(vCell.y));
}

template <class CellData>
ezVec3 ezGameGrid<CellData>::GetCellWorldSpaceOrigin(const ezVec2I32& Coord) const
{
  return m_vWorldSpaceOrigin + m_mRotateToWorldspace * GetCellLocalSpaceOrigin(Coord);
}

template <class CellData>
ezVec3 ezGameGrid<CellData>::GetCellLocalSpaceOrigin(const ezVec2I32& Coord) const
{
  return m_vLocalSpaceCellSize.CompMul(ezVec3((float)Coord.x, (float)Coord.y, 0.0f));
}

template <class CellData>
ezVec3 ezGameGrid<CellData>::GetCellWorldSpaceCenter(const ezVec2I32& Coord) const
{
  return m_vWorldSpaceOrigin + m_mRotateToWorldspace * GetCellLocalSpaceCenter(Coord);
}

template <class CellData>
ezVec3 ezGameGrid<CellData>::GetCellLocalSpaceCenter(const ezVec2I32& Coord) const
{
  return m_vLocalSpaceCellSize.CompMul(ezVec3((float)Coord.x + 0.5f, (float)Coord.y + 0.5f, 0.5f));
}

template <class CellData>
bool ezGameGrid<CellData>::IsValidCellCoordinate(const ezVec2I32& Coord) const
{
  return (Coord.x >= 0 && Coord.x < m_uiGridSizeX && Coord.y >= 0 && Coord.y < m_uiGridSizeY);
}

template <class CellData>
bool ezGameGrid<CellData>::PickCell(const ezVec3& vRayStartPos, const ezVec3& vRayDirNorm, ezVec2I32* out_CellCoord, ezVec3* out_vIntersection) const
{
  ezPlane p;
  p.SetFromNormalAndPoint(m_mRotateToWorldspace * ezVec3(0, 0, -1), m_vWorldSpaceOrigin);

  ezVec3 vPos;

  if (!p.GetRayIntersection(vRayStartPos, vRayDirNorm, nullptr, &vPos))
    return false;

  if (out_vIntersection)
    *out_vIntersection = vPos;

  if (out_CellCoord)
    *out_CellCoord = GetCellAtWorldPosition(vPos);

  return true;
}

template <class CellData>
ezBoundingBox ezGameGrid<CellData>::GetWorldBoundingBox() const
{
  ezVec3 vGridBox(m_uiGridSizeX, m_uiGridSizeY, 1.0f);

  vGridBox = m_mRotateToWorldspace * m_vLocalSpaceCellSize.CompMul(vGridBox);

  return ezBoundingBox(m_vWorldSpaceOrigin, m_vWorldSpaceOrigin + vGridBox);
}

template <class CellData>
bool ezGameGrid<CellData>::GetRayIntersection(const ezVec3& vRayStartWorldSpace, const ezVec3& vRayDirNormalizedWorldSpace, float fMaxLength,
  float& out_fIntersection, ezVec2I32& out_CellCoord) const
{
  const ezVec3 vRayStart = m_mRotateToGridspace * (vRayStartWorldSpace - m_vWorldSpaceOrigin);
  const ezVec3 vRayDir = m_mRotateToGridspace * vRayDirNormalizedWorldSpace;

  ezVec3 vGridBox(m_uiGridSizeX, m_uiGridSizeY, 1.0f);

  const ezBoundingBox localBox(ezVec3(0.0f), m_vLocalSpaceCellSize.CompMul(vGridBox));

  if (localBox.Contains(vRayStart))
  {
    // if the ray is already inside the box, we know that a cell is hit
    out_fIntersection = 0.0f;
  }
  else
  {
    if (!localBox.GetRayIntersection(vRayStart, vRayDir, &out_fIntersection, nullptr))
      return false;

    if (out_fIntersection > fMaxLength)
      return false;
  }

  const ezVec3 vEnterPos = vRayStart + vRayDir * out_fIntersection;

  const ezVec3 vCell = vEnterPos.CompMul(m_vInverseLocalSpaceCellSize);

  // Without the Floor, the border case when the position is outside (-1 / -1) is not immediately detected
  out_CellCoord = ezVec2I32((ezInt32)ezMath::Floor(vCell.x), (ezInt32)ezMath::Floor(vCell.y));
  out_CellCoord.x = ezMath::Clamp(out_CellCoord.x, 0, m_uiGridSizeX - 1);
  out_CellCoord.y = ezMath::Clamp(out_CellCoord.y, 0, m_uiGridSizeY - 1);

  return true;
}

template <class CellData>
bool ezGameGrid<CellData>::GetRayIntersectionExpandedBBox(const ezVec3& vRayStartWorldSpace, const ezVec3& vRayDirNormalizedWorldSpace,
  float fMaxLength, float& out_fIntersection, const ezVec3& vExpandBBoxByThis) const
{
  const ezVec3 vRayStart = m_mRotateToGridspace * (vRayStartWorldSpace - m_vWorldSpaceOrigin);
  const ezVec3 vRayDir = m_mRotateToGridspace * vRayDirNormalizedWorldSpace;

  ezVec3 vGridBox(m_uiGridSizeX, m_uiGridSizeY, 1.0f);

  ezBoundingBox localBox(ezVec3(0.0f), m_vLocalSpaceCellSize.CompMul(vGridBox));
  localBox.Grow(vExpandBBoxByThis);

  if (localBox.Contains(vRayStart))
  {
    // if the ray is already inside the box, we know that a cell is hit
    out_fIntersection = 0.0f;
  }
  else
  {
    if (!localBox.GetRayIntersection(vRayStart, vRayDir, &out_fIntersection, nullptr))
      return false;

    if (out_fIntersection > fMaxLength)
      return false;
  }

  return true;
}
