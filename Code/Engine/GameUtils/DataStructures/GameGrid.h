#pragma once

#include <GameUtils/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>

struct ezGridCoordinate
{
  ezGridCoordinate() { }
  ezGridCoordinate(ezInt32 X, ezInt32 Z, ezInt32 slice = 0) : x(X), z(Z), Slice(slice) { }

  ezInt32 x;
  ezInt32 z;
  ezInt32 Slice;
};

template<class CellData>
class ezGameGrid
{
public:

  void CreateGrid(ezUInt16 uiWidth, ezUInt16 uiDepth, ezUInt8 uiSlices = 1);

  void SetWorldDimensions(const ezVec3& vLowerLeftCorner, const ezVec3& vCellSize);

  /// \brief Returns the cell indices (x, slice, z) at the given world-space position. The world space dimension must be set for this to work.
  /// The indices might be outside valid ranges (negative, larger than the maximum size).
  ezGridCoordinate GetCellAtPosition(const ezVec3& vWorldSpacePos) const;

  ezUInt16 GetWidth() const  { return m_uiWidth;  }
  ezUInt16 GetDepth() const  { return m_uiDepth;  }
  ezUInt8  GetSlices() const { return m_uiSlices; }

  /// \brief Returns the world-space bounding box of the grid, as specified via SetWorldDimensions.
  ezBoundingBox GetBoundingBox() const;

  ezUInt32 GetNumCells() const;

  CellData& GetCellByIndex(ezUInt32 uiIndex);
  const CellData& GetCellByIndex(ezUInt32 uiIndex) const;

  CellData& GetCell(const ezGridCoordinate& Coord);
  const CellData& GetCell(const ezGridCoordinate& Coord) const;

  ezGridCoordinate GetCellCoordsByInex(ezUInt32 uiIndex) const;

  ezUInt32 GetCellIndex(const ezGridCoordinate& Coord) const;

  ezVec3 GetCellOrigin(const ezGridCoordinate& Coord) const;

  ezVec3 GetCellSize() const;

  bool IsValidCellCoordinate(const ezGridCoordinate& Coord) const;

  ezGridCoordinate PickCell(const ezVec3& vRayStartPos, const ezVec3& vRayDirNorm, ezVec3* out_vIntersection = NULL, ezUInt8 uiSlice = 0) const;

private:
  ezUInt16 m_uiWidth;
  ezUInt16 m_uiDepth;
  ezUInt8  m_uiSlices;

  ezVec3 m_vWorldSpaceOrigin;
  ezVec3 m_vWorldSpaceCellSize;
  ezVec3 m_vInverseWorldSpaceCellSize;

  ezDynamicArray<CellData> m_Cells;
};

#include <GameUtils/DataStructures/Implementation/GameGrid_inl.h>

