#include <PCH.h>
#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/SimdMath/SimdConversion.h>

namespace
{
  enum
  {
    MAX_CELL_INDEX = (1 << 20) - 1,
    CELL_INDEX_MASK = (1 << 21) - 1
  };

  EZ_ALWAYS_INLINE ezSimdVec4f ToVec3(const ezSimdVec4i& v)
  {
    return v.ToFloat();
  }

  EZ_ALWAYS_INLINE ezSimdVec4i ToVec3I32(const ezSimdVec4f& v)
  {
    ezSimdVec4f vf = v.Floor();
    return ezSimdVec4i::Truncate(vf);
  }

  EZ_ALWAYS_INLINE ezUInt64 GetCellKey(ezInt32 x, ezInt32 y, ezInt32 z)
  {
    ezUInt64 sx = (x + MAX_CELL_INDEX) & CELL_INDEX_MASK;
    ezUInt64 sy = (y + MAX_CELL_INDEX) & CELL_INDEX_MASK;
    ezUInt64 sz = (z + MAX_CELL_INDEX) & CELL_INDEX_MASK;

    return (sx << 42) | (sy << 21) | sz;
  }

  EZ_ALWAYS_INLINE ezSimdBBox ComputeCellBoundingBox(const ezSimdVec4i& cellIndex, const ezSimdVec4i& iCellSize)
  {
    ezSimdVec4i overlapSize = iCellSize >> 2;
    ezSimdVec4i minPos = cellIndex.CompMul(iCellSize);

    ezSimdVec4f bmin = ToVec3(minPos - overlapSize);
    ezSimdVec4f bmax = ToVec3(minPos + overlapSize + iCellSize);

    return ezSimdBBox(bmin, bmax);
  }

  struct PlaneData
  {
    ezSimdVec4f m_x0x1x2x3;
    ezSimdVec4f m_y0y1y2y3;
    ezSimdVec4f m_z0z1z2z3;
    ezSimdVec4f m_w0w1w2w3;

    ezSimdVec4f m_x4x5x4x5;
    ezSimdVec4f m_y4y5y4y5;
    ezSimdVec4f m_z4z5z4z5;
    ezSimdVec4f m_w4w5w4w5;
  };

  EZ_FORCE_INLINE bool SphereFrustumIntersect(const ezSimdBSphere& sphere, const PlaneData& planeData)
  {
    ezSimdVec4f pos_xxxx(sphere.m_CenterAndRadius.x());
    ezSimdVec4f pos_yyyy(sphere.m_CenterAndRadius.y());
    ezSimdVec4f pos_zzzz(sphere.m_CenterAndRadius.z());
    ezSimdVec4f pos_rrrr(sphere.m_CenterAndRadius.w());

    ezSimdVec4f dot_0123;
    dot_0123 = ezSimdVec4f::MulAdd(pos_xxxx, planeData.m_x0x1x2x3, planeData.m_w0w1w2w3);
    dot_0123 = ezSimdVec4f::MulAdd(pos_yyyy, planeData.m_y0y1y2y3, dot_0123);
    dot_0123 = ezSimdVec4f::MulAdd(pos_zzzz, planeData.m_z0z1z2z3, dot_0123);

    ezSimdVec4f dot_4545;
    dot_4545 = ezSimdVec4f::MulAdd(pos_xxxx, planeData.m_x4x5x4x5, planeData.m_w4w5w4w5);
    dot_4545 = ezSimdVec4f::MulAdd(pos_yyyy, planeData.m_y4y5y4y5, dot_4545);
    dot_4545 = ezSimdVec4f::MulAdd(pos_zzzz, planeData.m_z4z5z4z5, dot_4545);

    ezSimdVec4b cmp_0123 = dot_0123 > pos_rrrr;
    ezSimdVec4b cmp_4545 = dot_4545 > pos_rrrr;
    return (cmp_0123 || cmp_4545).NoneSet<4>();
  }

  EZ_FORCE_INLINE ezUInt32 SphereFrustumIntersect(const ezSimdBSphere& sphereA, const ezSimdBSphere& sphereB, const PlaneData& planeData)
  {
    ezSimdVec4f posA_xxxx(sphereA.m_CenterAndRadius.x());
    ezSimdVec4f posA_yyyy(sphereA.m_CenterAndRadius.y());
    ezSimdVec4f posA_zzzz(sphereA.m_CenterAndRadius.z());
    ezSimdVec4f posA_rrrr(sphereA.m_CenterAndRadius.w());

    ezSimdVec4f dotA_0123;
    dotA_0123 = ezSimdVec4f::MulAdd(posA_xxxx, planeData.m_x0x1x2x3, planeData.m_w0w1w2w3);
    dotA_0123 = ezSimdVec4f::MulAdd(posA_yyyy, planeData.m_y0y1y2y3, dotA_0123);
    dotA_0123 = ezSimdVec4f::MulAdd(posA_zzzz, planeData.m_z0z1z2z3, dotA_0123);

    ezSimdVec4f posB_xxxx(sphereB.m_CenterAndRadius.x());
    ezSimdVec4f posB_yyyy(sphereB.m_CenterAndRadius.y());
    ezSimdVec4f posB_zzzz(sphereB.m_CenterAndRadius.z());
    ezSimdVec4f posB_rrrr(sphereB.m_CenterAndRadius.w());

    ezSimdVec4f dotB_0123;
    dotB_0123 = ezSimdVec4f::MulAdd(posB_xxxx, planeData.m_x0x1x2x3, planeData.m_w0w1w2w3);
    dotB_0123 = ezSimdVec4f::MulAdd(posB_yyyy, planeData.m_y0y1y2y3, dotB_0123);
    dotB_0123 = ezSimdVec4f::MulAdd(posB_zzzz, planeData.m_z0z1z2z3, dotB_0123);

    ezSimdVec4f posAB_xxxx = posA_xxxx.GetCombined<ezSwizzle::XXXX>(posB_xxxx);
    ezSimdVec4f posAB_yyyy = posA_yyyy.GetCombined<ezSwizzle::XXXX>(posB_yyyy);
    ezSimdVec4f posAB_zzzz = posA_zzzz.GetCombined<ezSwizzle::XXXX>(posB_zzzz);
    ezSimdVec4f posAB_rrrr = posA_rrrr.GetCombined<ezSwizzle::XXXX>(posB_rrrr);

    ezSimdVec4f dot_A45B45;
    dot_A45B45 = ezSimdVec4f::MulAdd(posAB_xxxx, planeData.m_x4x5x4x5, planeData.m_w4w5w4w5);
    dot_A45B45 = ezSimdVec4f::MulAdd(posAB_yyyy, planeData.m_y4y5y4y5, dot_A45B45);
    dot_A45B45 = ezSimdVec4f::MulAdd(posAB_zzzz, planeData.m_z4z5z4z5, dot_A45B45);

    ezSimdVec4b cmp_A0123 = dotA_0123 > posA_rrrr;
    ezSimdVec4b cmp_B0123 = dotB_0123 > posB_rrrr;
    ezSimdVec4b cmp_A45B45 = dot_A45B45 > posAB_rrrr;

    ezSimdVec4b cmp_A45 = cmp_A45B45.Get<ezSwizzle::XYXY>();
    ezSimdVec4b cmp_B45 = cmp_A45B45.Get<ezSwizzle::ZWZW>();

    ezUInt32 result = (cmp_A0123 || cmp_A45).NoneSet<4>() ? 1 : 0;
    result |= (cmp_B0123 || cmp_B45).NoneSet<4>() ? 2 : 0;

    return result;
  }
}

//////////////////////////////////////////////////////////////////////////

ezSpatialSystem_RegularGrid::Cell::Cell(ezAllocatorBase* pAllocator, ezAllocatorBase* pAlignedAllocator)
  : m_BoundingSpheres(pAlignedAllocator)
  , m_DataPointers(pAllocator)
  , m_PointerToIndexTable(pAllocator)
{
}

EZ_FORCE_INLINE void ezSpatialSystem_RegularGrid::Cell::AddData(ezSpatialData* pData)
{
  ezUInt32 dataIndex = m_BoundingSpheres.GetCount();
  m_BoundingSpheres.PushBack(pData->m_Bounds.GetSphere());
  m_DataPointers.PushBack(pData);
  EZ_VERIFY(!m_PointerToIndexTable.Insert(pData, dataIndex), "Implementation error");

  EZ_ASSERT_DEBUG(pData->m_pUserData == nullptr, "Data can't be in multiple cells");
  pData->m_pUserData = this;
}

EZ_FORCE_INLINE void ezSpatialSystem_RegularGrid::Cell::RemoveData(ezSpatialData* pData)
{
  ezUInt32 dataIndex = 0;
  EZ_VERIFY(m_PointerToIndexTable.Remove(pData, &dataIndex), "Implementation error");

  if (dataIndex != m_DataPointers.GetCount() - 1)
  {
    ezSpatialData* pLastData = m_DataPointers.PeekBack();
    m_PointerToIndexTable[pLastData] = dataIndex;
  }

  m_BoundingSpheres.RemoveAtSwap(dataIndex);
  m_DataPointers.RemoveAtSwap(dataIndex);

  EZ_ASSERT_DEBUG(pData->m_pUserData == this, "Implementation error");
  pData->m_pUserData = nullptr;
}

EZ_FORCE_INLINE void ezSpatialSystem_RegularGrid::Cell::UpdateData(ezSpatialData* pData)
{
  ezUInt32 dataIndex = 0;
  EZ_VERIFY(m_PointerToIndexTable.TryGetValue(pData, dataIndex), "Implementation error");

  m_BoundingSpheres[dataIndex] = pData->m_Bounds.GetSphere();
}

ezBoundingBox ezSpatialSystem_RegularGrid::Cell::GetBoundingBox() const
{
  return ezSimdConversion::ToBBoxSphere(m_Bounds).GetBox();
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpatialSystem_RegularGrid, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSpatialSystem_RegularGrid::ezSpatialSystem_RegularGrid(ezUInt32 uiCellSize /* = 128 */)
  : m_AlignedAllocator("Spatial System Aligned", ezFoundation::GetAlignedAllocator())
  , m_iCellSize(uiCellSize)
  , m_fOverlapSize(uiCellSize / 4.0f)
  , m_fInvCellSize(1.0f / uiCellSize)
{
  ezSimdBBox overflowBox;
  overflowBox.SetCenterAndHalfExtents(ezSimdVec4f::ZeroVector(), ezSimdVec4f((float)(uiCellSize * MAX_CELL_INDEX)));

  m_pOverflowCell = EZ_NEW(&m_AlignedAllocator, Cell, &m_Allocator, &m_AlignedAllocator);
  m_pOverflowCell->m_Bounds = overflowBox;
}

ezSpatialSystem_RegularGrid::~ezSpatialSystem_RegularGrid()
{
  EZ_DELETE(&m_AlignedAllocator, m_pOverflowCell);

  for (auto it = m_Cells.GetIterator(); it.IsValid(); ++it)
  {
    Cell* pCell = it.Value();
    EZ_DELETE(&m_AlignedAllocator, pCell);
  }
}

ezResult ezSpatialSystem_RegularGrid::GetCellBoxForSpatialData(const ezSpatialDataHandle& hData, ezBoundingBox& out_BoundingBox) const
{
  ezSpatialData* pData;
  if (!m_DataTable.TryGetValue(hData.GetInternalID(), pData))
    return EZ_FAILURE;

#if 0

  for (auto it = m_Cells.GetIterator(); it.IsValid(); ++it)
  {
    Cell& cell = *it.Value();
    if (cell.m_PointerToIndexTable.Contains(pData))
    {
      out_BoundingBox = cell.GetBoundingBox();
      return EZ_SUCCESS;
    }
  }

  if (m_pOverflowCell->m_PointerToIndexTable.Contains(pData))
  {
    out_BoundingBox = m_pOverflowCell->GetBoundingBox();
    return EZ_SUCCESS;
  }

#else

  if (Cell* pCell = static_cast<Cell*>(pData->m_pUserData))
  {
    out_BoundingBox = pCell->GetBoundingBox();
    return EZ_SUCCESS;
  }

#endif

  return EZ_FAILURE;
}

void ezSpatialSystem_RegularGrid::GetAllCellBoxes(ezHybridArray<ezBoundingBox, 16>& out_BoundingBoxes) const
{
  for (auto it = m_Cells.GetIterator(); it.IsValid(); ++it)
  {
    Cell& cell = *it.Value();
    out_BoundingBoxes.ExpandAndGetRef() = cell.GetBoundingBox();
  }
}

void ezSpatialSystem_RegularGrid::FindObjectsInSphereInternal(const ezBoundingSphere& sphere, QueryCallback callback, QueryStats* pStats) const
{
  ezSimdBSphere simdSphere(ezSimdConversion::ToVec3(sphere.m_vCenter), sphere.m_fRadius);
  ezSimdBBox simdBox;
  simdBox.SetCenterAndHalfExtents(simdSphere.m_CenterAndRadius, simdSphere.m_CenterAndRadius.Get<ezSwizzle::WWWW>());

  ForEachCellInBox(simdBox, [&](const ezSimdVec4i& cellIndex, ezUInt64 cellKey, Cell* pCell)
  {
    if (pCell != nullptr)
    {
      ezSimdBBox cellBox = pCell->m_Bounds.GetBox();
      if (cellBox.Overlaps(simdSphere))
      {
        const ezUInt32 numSpheres = pCell->m_BoundingSpheres.GetCount();

        #if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
          if (pStats != nullptr)
          {
            pStats->m_uiNumObjectsTested += numSpheres;
          }
        #endif

        for (ezUInt32 i = 0; i < numSpheres; ++i)
        {
          auto& objectSphere = pCell->m_BoundingSpheres[i];
          if (!simdSphere.Overlaps(objectSphere))
            continue;

          ezSpatialData* pData = pCell->m_DataPointers[i];

          // TODO: The return value has to have more control
          if (callback(pData->m_pObject) == ezVisitorExecution::Stop)
            return;

          #if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
            if (pStats != nullptr)
            {
              pStats->m_uiNumObjectsPassed++;
            }
          #endif
        }
      }
    }
  });
}

void ezSpatialSystem_RegularGrid::FindObjectsInBoxInternal(const ezBoundingBox& box, QueryCallback callback, QueryStats* pStats) const
{
  ezSimdBBox simdBox(ezSimdConversion::ToVec3(box.m_vMin), ezSimdConversion::ToVec3(box.m_vMax));

  ForEachCellInBox(simdBox, [&](const ezSimdVec4i& cellIndex, ezUInt64 cellKey, Cell* pCell)
  {
    if (pCell != nullptr)
    {
      const ezUInt32 numSpheres = pCell->m_BoundingSpheres.GetCount();

      #if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
        if (pStats != nullptr)
        {
          pStats->m_uiNumObjectsTested += numSpheres;
        }
      #endif

      for (ezUInt32 i = 0; i < numSpheres; ++i)
      {
        auto& objectSphere = pCell->m_BoundingSpheres[i];
        if (!simdBox.Overlaps(objectSphere))
          continue;

        ezSpatialData* pData = pCell->m_DataPointers[i];
        if (!simdBox.Overlaps(pData->m_Bounds.GetBox()))
          continue;

        // TODO: The return value has to have more control
        if (callback(pData->m_pObject) == ezVisitorExecution::Stop)
          return;

        #if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
          if (pStats != nullptr)
          {
            pStats->m_uiNumObjectsPassed++;
          }
        #endif
      }
    }
  });
}

void ezSpatialSystem_RegularGrid::FindVisibleObjectsInternal(const ezFrustum& frustum, ezDynamicArray<const ezGameObject*>& out_Objects, QueryStats* pStats) const
{
  ezVec3 cornerPoints[8];
  frustum.ComputeCornerPoints(cornerPoints);

  ezSimdVec4f simdCornerPoints[8];
  for (ezUInt32 i = 0; i < 8; ++i)
  {
    simdCornerPoints[i] = ezSimdConversion::ToVec3(cornerPoints[i]);
  }

  ezSimdBBox simdBox;
  simdBox.SetFromPoints(simdCornerPoints, 8);

  PlaneData planeData;
  {
    // Compiler is too stupid to properly unroll a constant loop so we do it by hand
    ezSimdVec4f plane0 = ezSimdConversion::ToVec4(*reinterpret_cast<const ezVec4*>(&(frustum.GetPlane(0).m_vNormal.x)));
    ezSimdVec4f plane1 = ezSimdConversion::ToVec4(*reinterpret_cast<const ezVec4*>(&(frustum.GetPlane(1).m_vNormal.x)));
    ezSimdVec4f plane2 = ezSimdConversion::ToVec4(*reinterpret_cast<const ezVec4*>(&(frustum.GetPlane(2).m_vNormal.x)));
    ezSimdVec4f plane3 = ezSimdConversion::ToVec4(*reinterpret_cast<const ezVec4*>(&(frustum.GetPlane(3).m_vNormal.x)));
    ezSimdVec4f plane4 = ezSimdConversion::ToVec4(*reinterpret_cast<const ezVec4*>(&(frustum.GetPlane(4).m_vNormal.x)));
    ezSimdVec4f plane5 = ezSimdConversion::ToVec4(*reinterpret_cast<const ezVec4*>(&(frustum.GetPlane(5).m_vNormal.x)));

    ezSimdMat4f helperMat;
    helperMat.SetRows(plane0, plane1, plane2, plane3);

    planeData.m_x0x1x2x3 = helperMat.m_col0;
    planeData.m_y0y1y2y3 = helperMat.m_col1;
    planeData.m_z0z1z2z3 = helperMat.m_col2;
    planeData.m_w0w1w2w3 = helperMat.m_col3;

    helperMat.SetRows(plane4, plane5, plane4, plane5);

    planeData.m_x4x5x4x5 = helperMat.m_col0;
    planeData.m_y4y5y4y5 = helperMat.m_col1;
    planeData.m_z4z5z4z5 = helperMat.m_col2;
    planeData.m_w4w5w4w5 = helperMat.m_col3;
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  ezUInt32 uiNumObjectsTested = 0;
  ezUInt32 uiNumObjectsPassed = 0;
#endif

  ForEachCellInBox(simdBox, [&](const ezSimdVec4i& cellIndex, ezUInt64 cellKey, Cell* pCell)
  {
    if (pCell == nullptr)
      return;

    ezSimdBSphere cellSphere = pCell->m_Bounds.GetSphere();
    if (!SphereFrustumIntersect(cellSphere, planeData))
      return;

    const ezUInt32 numSpheres = pCell->m_BoundingSpheres.GetCount();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    uiNumObjectsTested += numSpheres;
#endif
    ezUInt32 currentIndex = 0;

    while (currentIndex < numSpheres)
    {
      if (numSpheres - currentIndex >= 32)
      {
        ezUInt32 mask = 0;

        for (ezUInt32 i = 0; i < 32; i += 2)
        {
          auto& objectSphereA = pCell->m_BoundingSpheres[currentIndex + i + 0];
          auto& objectSphereB = pCell->m_BoundingSpheres[currentIndex + i + 1];

          mask |= SphereFrustumIntersect(objectSphereA, objectSphereB, planeData) << i;
        }

        while (mask > 0)
        {
          ezUInt32 i = ezMath::FirstBitLow(mask);
          mask &= ~(1 << i);

          ezSpatialData* pData = pCell->m_DataPointers[currentIndex + i];
          out_Objects.PushBack(pData->m_pObject);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
          uiNumObjectsPassed++;
#endif
        }

        currentIndex += 32;
      }
      else
      {
        ezUInt32 i = currentIndex;
        ++currentIndex;

        auto& objectSphere = pCell->m_BoundingSpheres[i];
        if (!SphereFrustumIntersect(objectSphere, planeData))
          continue;

        ezSpatialData* pData = pCell->m_DataPointers[i];
        out_Objects.PushBack(pData->m_pObject);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
        uiNumObjectsPassed++;
#endif
      }
    }
  });

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (pStats != nullptr)
  {
    pStats->m_uiNumObjectsTested = uiNumObjectsTested;
    pStats->m_uiNumObjectsPassed = uiNumObjectsPassed;
  }
#endif
}

void ezSpatialSystem_RegularGrid::SpatialDataAdded(ezSpatialData* pData)
{
  Cell* pCell = GetOrCreateCell(pData->m_Bounds);
  pCell->AddData(pData);
}

void ezSpatialSystem_RegularGrid::SpatialDataRemoved(ezSpatialData* pData)
{
  Cell* pCell = static_cast<Cell*>(pData->m_pUserData);
  pCell->RemoveData(pData);
}

void ezSpatialSystem_RegularGrid::SpatialDataChanged(ezSpatialData* pData, const ezSimdBBoxSphere& oldBounds)
{
  Cell* pOldCell = static_cast<Cell*>(pData->m_pUserData);
  Cell* pNewCell = GetOrCreateCell(pData->m_Bounds);

  if (pOldCell == pNewCell)
  {
    pOldCell->UpdateData(pData);
  }
  else
  {
    pOldCell->RemoveData(pData);
    pNewCell->AddData(pData);
  }
}

void ezSpatialSystem_RegularGrid::FixSpatialDataPointer(ezSpatialData* pOldPtr, ezSpatialData* pNewPtr)
{
  Cell* pCell = static_cast<Cell*>(pNewPtr->m_pUserData);

  ezUInt32 dataIndex = 0;
  EZ_VERIFY(pCell->m_PointerToIndexTable.Remove(pOldPtr, &dataIndex), "Implementation error");

  pCell->m_DataPointers[dataIndex] = pNewPtr;
  pCell->m_PointerToIndexTable.Insert(pNewPtr, dataIndex);
}

template <typename Functor>
EZ_FORCE_INLINE void ezSpatialSystem_RegularGrid::ForEachCellInBox(const ezSimdBBox& box, Functor func) const
{
  ezSimdVec4i minIndex = ToVec3I32((box.m_Min - m_fOverlapSize) * m_fInvCellSize);
  ezSimdVec4i maxIndex = ToVec3I32((box.m_Max + m_fOverlapSize) * m_fInvCellSize);

  EZ_ASSERT_DEBUG((minIndex.Abs() < ezSimdVec4i(MAX_CELL_INDEX)).AllSet<3>(), "Position is too big");
  EZ_ASSERT_DEBUG((maxIndex.Abs() < ezSimdVec4i(MAX_CELL_INDEX)).AllSet<3>(), "Position is too big");

  const ezInt32 iMinX = minIndex.x();
  const ezInt32 iMinY = minIndex.y();
  const ezInt32 iMinZ = minIndex.z();

#if 0

  const ezInt32 iMaxX = maxIndex.x();
  const ezInt32 iMaxY = maxIndex.y();
  const ezInt32 iMaxZ = maxIndex.z();

  for (ezInt32 z = iMinZ; z <= iMaxZ; ++z)
  {
    for (ezInt32 y = iMinY; y <= iMaxY; ++y)
    {
      for (ezInt32 x = iMinX; x <= iMaxX; ++x)
      {
        ezUInt64 cellKey = GetCellKey(x, y, z);

        Cell* pCell = nullptr;
        m_Cells.TryGetValue(cellKey, pCell);

        ezSimdVec4i cellIndex(x, y, z);
        func(cellIndex, cellKey, pCell);
      }
    }
  }

#else

  const ezSimdVec4i diff = maxIndex - minIndex + ezSimdVec4i(1);
  const ezInt32 iDiffX = diff.x();
  const ezInt32 iDiffY = diff.y();
  const ezInt32 iDiffZ = diff.z();
  const ezInt32 iNumIterations = iDiffX * iDiffY * iDiffZ;

  for (ezInt32 i = 0; i < iNumIterations; ++i)
  {
    ezInt32 index = i;
    ezInt32 z = i / (iDiffX * iDiffY);
    index -= z * iDiffX * iDiffY;
    ezInt32 y = index / iDiffX;
    ezInt32 x = index - (y * iDiffX);

    x += iMinX;
    y += iMinY;
    z += iMinZ;

    ezUInt64 cellKey = GetCellKey(x, y, z);

    Cell* pCell = nullptr;
    m_Cells.TryGetValue(cellKey, pCell);

    ezSimdVec4i cellIndex(x, y, z);
    func(cellIndex, cellKey, pCell);
  }

  func(ezSimdVec4i::ZeroVector(), 0, m_pOverflowCell);

#endif
}

ezSpatialSystem_RegularGrid::Cell* ezSpatialSystem_RegularGrid::GetOrCreateCell(const ezSimdBBoxSphere& bounds)
{
  ezSimdVec4i cellIndex = ToVec3I32(bounds.m_CenterAndRadius * m_fInvCellSize);
  ezSimdBBox cellBox = ComputeCellBoundingBox(cellIndex, m_iCellSize);

  if (cellBox.Contains(bounds.GetBox()))
  {
    ezUInt64 cellKey = GetCellKey(cellIndex.x(), cellIndex.y(), cellIndex.z());

    Cell* pCell = nullptr;
    if (!m_Cells.TryGetValue(cellKey, pCell))
    {
      pCell = EZ_NEW(&m_AlignedAllocator, Cell, &m_Allocator, &m_AlignedAllocator);
      pCell->m_Bounds = cellBox;

      m_Cells.Insert(cellKey, pCell);
    }

    return pCell;
  }
  else
  {
    return m_pOverflowCell;
  }
}



EZ_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem_RegularGrid);

