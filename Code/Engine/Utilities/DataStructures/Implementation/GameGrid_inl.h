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
      mRot = ezMat3::MakeAxisRotation(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(90.0f));
      break;
    case InPlaneXminusZ:
      mRot = ezMat3::MakeAxisRotation(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(-90.0f));
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
ezVec3 ezGameGrid<CellData>::GetCellWorldSpaceOrigin(const ezVec2I32& vCoord) const
{
  return m_vWorldSpaceOrigin + m_mRotateToWorldspace * GetCellLocalSpaceOrigin(vCoord);
}

template <class CellData>
ezVec3 ezGameGrid<CellData>::GetCellLocalSpaceOrigin(const ezVec2I32& vCoord) const
{
  return m_vLocalSpaceCellSize.CompMul(ezVec3((float)vCoord.x, (float)vCoord.y, 0.0f));
}

template <class CellData>
ezVec3 ezGameGrid<CellData>::GetCellWorldSpaceCenter(const ezVec2I32& vCoord) const
{
  return m_vWorldSpaceOrigin + m_mRotateToWorldspace * GetCellLocalSpaceCenter(vCoord);
}

template <class CellData>
ezVec3 ezGameGrid<CellData>::GetCellLocalSpaceCenter(const ezVec2I32& vCoord) const
{
  return m_vLocalSpaceCellSize.CompMul(ezVec3((float)vCoord.x + 0.5f, (float)vCoord.y + 0.5f, 0.5f));
}

template <class CellData>
bool ezGameGrid<CellData>::IsValidCellCoordinate(const ezVec2I32& vCoord) const
{
  return (vCoord.x >= 0 && vCoord.x < m_uiGridSizeX && vCoord.y >= 0 && vCoord.y < m_uiGridSizeY);
}

template <class CellData>
bool ezGameGrid<CellData>::PickCell(const ezVec3& vRayStartPos, const ezVec3& vRayDirNorm, ezVec2I32* out_pCellCoord, ezVec3* out_pIntersection) const
{
  ezPlane p;
  p = ezPlane::MakeFromNormalAndPoint(m_mRotateToWorldspace * ezVec3(0, 0, -1), m_vWorldSpaceOrigin);

  ezVec3 vPos;

  if (!p.GetRayIntersection(vRayStartPos, vRayDirNorm, nullptr, &vPos))
    return false;

  if (out_pIntersection)
    *out_pIntersection = vPos;

  if (out_pCellCoord)
    *out_pCellCoord = GetCellAtWorldPosition(vPos);

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
  float& out_fIntersection, ezVec2I32& out_vCellCoord) const
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
  out_vCellCoord = ezVec2I32((ezInt32)ezMath::Floor(vCell.x), (ezInt32)ezMath::Floor(vCell.y));
  out_vCellCoord.x = ezMath::Clamp(out_vCellCoord.x, 0, m_uiGridSizeX - 1);
  out_vCellCoord.y = ezMath::Clamp(out_vCellCoord.y, 0, m_uiGridSizeY - 1);

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

template <class CellData>
void ezGameGrid<CellData>::ComputeWorldSpaceCorners(ezVec3* pCorners) const
{
  pCorners[0] = m_vWorldSpaceOrigin;
  pCorners[1] = m_vWorldSpaceOrigin + m_mRotateToWorldspace * ezVec3(m_uiGridSizeX * m_vLocalSpaceCellSize.x, 0, 0);
  pCorners[2] = m_vWorldSpaceOrigin + m_mRotateToWorldspace * ezVec3(0, m_uiGridSizeY * m_vLocalSpaceCellSize.y, 0);
  pCorners[3] = m_vWorldSpaceOrigin + m_mRotateToWorldspace * ezVec3(m_uiGridSizeX * m_vLocalSpaceCellSize.x, m_uiGridSizeY * m_vLocalSpaceCellSize.y, 0);
}
