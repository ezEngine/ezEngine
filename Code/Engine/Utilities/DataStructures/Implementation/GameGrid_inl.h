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
ezVec3 ezGameGrid<CellData>::GetCellWorldSpaceCenter(const ezVec2I32& vCoord, float fHeight) const
{
  return m_vWorldSpaceOrigin + m_mRotateToWorldspace * GetCellLocalSpaceCenter(vCoord, fHeight);
}

template <class CellData>
ezVec3 ezGameGrid<CellData>::GetCellLocalSpaceCenter(const ezVec2I32& vCoord, float fHeight) const
{
  return m_vLocalSpaceCellSize.CompMul(ezVec3((float)vCoord.x + 0.5f, (float)vCoord.y + 0.5f, fHeight));
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

  if (!p.GetRayIntersectionBiDirectional(vRayStartPos, vRayDirNorm, nullptr, &vPos))
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


template <class CellData>
ezResult ezGameGrid<CellData>::Serialize(ezStreamWriter& ref_stream) const
{
  auto& stream = ref_stream;

  stream.WriteVersion(1);

  stream << m_uiGridSizeX;
  stream << m_uiGridSizeY;
  stream << m_mRotateToWorldspace;
  stream << m_mRotateToGridspace;
  stream << m_vWorldSpaceOrigin;
  stream << m_vLocalSpaceCellSize;
  stream << m_vInverseLocalSpaceCellSize;
  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_Cells));

  return EZ_SUCCESS;
}

template <class CellData>
ezResult ezGameGrid<CellData>::Deserialize(ezStreamReader& ref_stream)
{
  auto& stream = ref_stream;

  const ezTypeVersion version = stream.ReadVersion(1);
  EZ_IGNORE_UNUSED(version);

  stream >> m_uiGridSizeX;
  stream >> m_uiGridSizeY;
  stream >> m_mRotateToWorldspace;
  stream >> m_mRotateToGridspace;
  stream >> m_vWorldSpaceOrigin;
  stream >> m_vLocalSpaceCellSize;
  stream >> m_vInverseLocalSpaceCellSize;
  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_Cells));

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template <class CellData, class EdgeData>
void ezGameGridWithEdges<CellData, EdgeData>::ConvertEdgeIndexToCellCoords(ezUInt32 uiEdgeIndex, ezVec2I32& out_vCell1, ezVec2I32& out_vCell2) const
{
  const ezUInt32 uiOffsetY = (this->m_uiGridSizeX + 1) * this->m_uiGridSizeY;


  if (uiEdgeIndex < uiOffsetY)
  {
    out_vCell1.y = uiEdgeIndex / (this->m_uiGridSizeX + 1);
    out_vCell1.x = uiEdgeIndex - out_vCell1.y * (this->m_uiGridSizeX + 1);

    out_vCell2 = out_vCell1;
    out_vCell2.x += 1;
  }
  else
  {
    uiEdgeIndex -= uiOffsetY;

    out_vCell1.x = uiEdgeIndex / (this->m_uiGridSizeY + 1);
    out_vCell1.y = uiEdgeIndex - out_vCell1.x * (this->m_uiGridSizeY + 1);

    out_vCell2 = out_vCell1;
    out_vCell2.y += 1;
  }
}

template <class CellData, class EdgeData>
ezUInt32 ezGameGridWithEdges<CellData, EdgeData>::ConvertCellCoordinateToEdgeIndex(const ezVec2I32& vCoord, ezGameGridCellEdge edge) const
{
  const ezUInt32 uiOffsetY = (this->m_uiGridSizeX + 1) * this->m_uiGridSizeY;

  switch (edge)
  {
    case ezGameGridCellEdge::NegX:
      return vCoord.y * (this->m_uiGridSizeX + 1) + vCoord.x;

    case ezGameGridCellEdge::PosX:
      return vCoord.y * (this->m_uiGridSizeX + 1) + vCoord.x + 1;

    case ezGameGridCellEdge::NegY:
      return uiOffsetY + vCoord.x * (this->m_uiGridSizeY + 1) + vCoord.y;

    case ezGameGridCellEdge::PosY:
      return uiOffsetY + vCoord.x * (this->m_uiGridSizeY + 1) + vCoord.y + 1;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

template <class CellData, class EdgeData>
void ezGameGridWithEdges<CellData, EdgeData>::CreateGrid(ezUInt16 uiSizeX, ezUInt16 uiSizeY)
{
  ezGameGrid<CellData>::CreateGrid(uiSizeX, uiSizeY);

  m_Edges.Clear();
  m_Edges.SetCount(uiSizeX * uiSizeY * 2 + uiSizeX + uiSizeY);
}

template <class CellData, class EdgeData>
ezResult ezGameGridWithEdges<CellData, EdgeData>::Serialize(ezStreamWriter& ref_stream) const
{
  auto& stream = ref_stream;

  ezGameGrid<CellData>::Serialize(stream);

  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_Edges));

  return EZ_SUCCESS;
}

template <class CellData, class EdgeData>
ezResult ezGameGridWithEdges<CellData, EdgeData>::Deserialize(ezStreamReader& ref_stream)
{
  auto& stream = ref_stream;

  ezGameGrid<CellData>::Deserialize(stream);

  const ezTypeVersion version = stream.ReadVersion(1);
  EZ_IGNORE_UNUSED(version);

  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_Edges));

  return EZ_SUCCESS;
}


template <class CellData, class EdgeData>
ezVec3 ezGameGridWithEdges<CellData, EdgeData>::GetEdgeWorldSpaceCenter(ezUInt32 uiEdgeIndex, float fHeight) const
{
  return this->m_vWorldSpaceOrigin + this->m_mRotateToWorldspace * GetEdgeLocalSpaceCenter(uiEdgeIndex, fHeight);
}

template <class CellData, class EdgeData>
ezVec3 ezGameGridWithEdges<CellData, EdgeData>::GetEdgeLocalSpaceCenter(ezUInt32 uiEdgeIndex, float fHeight) const
{
  const ezUInt32 uiOffsetY = (this->m_uiGridSizeX + 1) * this->m_uiGridSizeY;

  if (uiEdgeIndex < uiOffsetY)
  {
    // Horizontal edge (parallel to X axis)
    const ezUInt32 y = uiEdgeIndex / (this->m_uiGridSizeX + 1);
    const ezUInt32 x = uiEdgeIndex - y * (this->m_uiGridSizeX + 1);

    return this->m_vLocalSpaceCellSize.CompMul(ezVec3((float)x, (float)y + 0.5f, fHeight));
  }
  else
  {
    // Vertical edge (parallel to Y axis)
    const ezUInt32 uiAdjustedIndex = uiEdgeIndex - uiOffsetY;
    const ezUInt32 x = uiAdjustedIndex / (this->m_uiGridSizeY + 1);
    const ezUInt32 y = uiAdjustedIndex - x * (this->m_uiGridSizeY + 1);

    return this->m_vLocalSpaceCellSize.CompMul(ezVec3((float)x + 0.5f, (float)y, fHeight));
  }
}

template <class CellData, class EdgeData>
bool ezGameGridWithEdges<CellData, EdgeData>::IsValidEdgeIndex(ezUInt32 uiEdgeIndex) const
{
  return uiEdgeIndex < m_Edges.GetCount();
}

template <class CellData, class EdgeData>
bool ezGameGridWithEdges<CellData, EdgeData>::IsEdgeHorizontal(ezUInt32 uiEdgeIndex) const
{
  const ezUInt32 uiOffsetY = (this->m_uiGridSizeX + 1) * this->m_uiGridSizeY;
  return uiEdgeIndex < uiOffsetY;
}

template <class CellData, class EdgeData>
bool ezGameGridWithEdges<CellData, EdgeData>::GetRayIntersectionWithEdge(const ezVec3& vRayStartWorldSpace,
  const ezVec3& vRayDirNormalizedWorldSpace, float fMaxLength, float& out_fIntersection, ezVec2I32& out_vCellCoord, ezGameGridCellEdge& out_edge) const
{
  // First find which cell is hit
  if (!this->GetRayIntersection(vRayStartWorldSpace, vRayDirNormalizedWorldSpace, fMaxLength, out_fIntersection, out_vCellCoord))
    return false;

  // Calculate the intersection point in world space
  const ezVec3 vIntersectionWorld = vRayStartWorldSpace + vRayDirNormalizedWorldSpace * out_fIntersection;

  // Transform intersection point to local space
  const ezVec3 vIntersectionLocal = this->m_mRotateToGridspace * (vIntersectionWorld - this->m_vWorldSpaceOrigin);

  // Get the cell center in local space
  const ezVec3 vCellCenter = this->GetCellLocalSpaceCenter(out_vCellCoord);

  // Calculate the offset from cell center to intersection point
  const ezVec3 vOffset = vIntersectionLocal - vCellCenter;

  // Determine which edge is closest based on the offset direction
  // Compare absolute values to find the dominant axis
  const float fAbsX = ezMath::Abs(vOffset.x);
  const float fAbsY = ezMath::Abs(vOffset.y);

  if (fAbsX > fAbsY)
  {
    // Closer to a vertical edge (NegX or PosX)
    out_edge = (vOffset.x < 0.0f) ? ezGameGridCellEdge::NegX : ezGameGridCellEdge::PosX;
  }
  else
  {
    // Closer to a horizontal edge (NegY or PosY)
    out_edge = (vOffset.y < 0.0f) ? ezGameGridCellEdge::NegY : ezGameGridCellEdge::PosY;
  }

  return true;
}
