#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Utilities/UtilitiesDLL.h>

/// \brief ezGameGrid is a general purpose 2D grid structure that has several convenience functions which are often required when working
/// with a grid.
template <class CellData>
class ezGameGrid
{
public:
  enum Orientation
  {
    InPlaneXY,      ///< The grid is expected to lie in the XY plane in world-space (when Y is up, this is similar to a 2D side scroller)
    InPlaneXZ,      ///< The grid is expected to lie in the XZ plane in world-space (when Y is up, this is similar to a top down RTS game)
    InPlaneXminusZ, ///< The grid is expected to lie in the XZ plane in world-space (when Y is up, this is similar to a top down RTS game)
  };

  ezGameGrid();

  /// \brief Clears all data and reallocates the grid with the given dimensions.
  void CreateGrid(ezUInt16 uiSizeX, ezUInt16 uiSizeY);

  /// \brief Sets the lower left position of the grid in world space coordinates and the cell size.
  ///
  /// Together with the grid size, these values determine the final world space dimensions.
  /// The rotation defines how the grid is rotated in world space. An identity rotation means that grid cell coordinates (X, Y)
  /// map directly to world space coordinates (X, Y). So the grid is 'standing up' in world space (considering that Y is 'up').
  /// Other rotations allow to rotate the grid into other planes, such as XZ, if that is more convenient.
  void SetWorldSpaceDimensions(const ezVec3& vLowerLeftCorner, const ezVec3& vCellSize, Orientation ori = InPlaneXZ);

  /// \brief Sets the lower left position of the grid in world space coordinates and the cell size.
  ///
  /// Together with the grid size, these values determine the final world space dimensions.
  /// The rotation defines how the grid is rotated in world space. An identity rotation means that grid cell coordinates (X, Y)
  /// map directly to world space coordinates (X, Y). So the grid is 'standing up' in world space (considering that Y is 'up').
  /// Other rotations allow to rotate the grid into other planes, such as XZ, if that is more convenient.
  void SetWorldSpaceDimensions(const ezVec3& vLowerLeftCorner, const ezVec3& vCellSize, const ezMat3& mRotation);

  /// \brief Returns the size of each cell.
  ezVec3 GetCellSize() const { return m_vLocalSpaceCellSize; }

  /// \brief Returns the coordinate of the cell at the given world-space position. The world space dimension must be set for this to work.
  /// The indices might be outside valid ranges (negative, larger than the maximum size).
  ezVec2I32 GetCellAtWorldPosition(const ezVec3& vWorldSpacePos) const;

  /// \brief Returns the number of cells along the X axis.
  ezUInt16 GetGridSizeX() const { return m_uiGridSizeX; }

  /// \brief Returns the number of cells along the Y axis.
  ezUInt16 GetGridSizeY() const { return m_uiGridSizeY; }

  /// \brief Returns the world-space bounding box of the grid, as specified via SetWorldDimensions.
  ezBoundingBox GetWorldBoundingBox() const;

  /// \brief Returns the total number of cells.
  ezUInt32 GetNumCells() const { return m_uiGridSizeX * m_uiGridSizeY; }

  /// \brief Gives access to a cell by cell index.
  CellData& GetCell(ezUInt32 uiIndex) { return m_Cells[uiIndex]; }

  /// \brief Gives access to a cell by cell index.
  const CellData& GetCell(ezUInt32 uiIndex) const { return m_Cells[uiIndex]; }

  /// \brief Gives access to a cell by cell coordinates.
  CellData& GetCell(const ezVec2I32& vCoord) { return m_Cells[ConvertCellCoordinateToIndex(vCoord)]; }

  /// \brief Gives access to a cell by cell coordinates.
  const CellData& GetCell(const ezVec2I32& vCoord) const { return m_Cells[ConvertCellCoordinateToIndex(vCoord)]; }

  /// \brief Converts a cell index into a 2D cell coordinate.
  ezVec2I32 ConvertCellIndexToCoordinate(ezUInt32 uiIndex) const { return ezVec2I32(uiIndex % m_uiGridSizeX, uiIndex / m_uiGridSizeX); }

  /// \brief Converts a cell coordinate into a cell index.
  ezUInt32 ConvertCellCoordinateToIndex(const ezVec2I32& vCoord) const { return vCoord.y * m_uiGridSizeX + vCoord.x; }

  /// \brief Returns the lower left world space position of the cell with the given coordinates.
  ezVec3 GetCellWorldSpaceOrigin(const ezVec2I32& vCoord) const;
  ezVec3 GetCellLocalSpaceOrigin(const ezVec2I32& vCoord) const;

  /// \brief Returns the center world space position of the cell with the given coordinates.
  ezVec3 GetCellWorldSpaceCenter(const ezVec2I32& vCoord) const;
  ezVec3 GetCellLocalSpaceCenter(const ezVec2I32& vCoord) const;

  /// \brief Checks whether the given cell coordinate is inside valid ranges.
  bool IsValidCellCoordinate(const ezVec2I32& vCoord) const;

  /// \brief Casts a world space ray through the grid and determines which cell is hit (if any).
  ///
  /// \note The picked cell is determined from where the ray hits the 'ground plane', ie. the plane that goes through the world space origin.
  /// \note Returns true, if the ray would hit the ground, at all. The returned cell coordinate may not be valid (outside the valid range).
  ///       Call IsValidCellCoordinate() to check.
  bool PickCell(const ezVec3& vRayStartPos, const ezVec3& vRayDirNorm, ezVec2I32* out_pCellCoord, ezVec3* out_pIntersection = nullptr) const;

  /// \brief Returns the lower left corner position in world space of the grid
  const ezVec3& GetWorldSpaceOrigin() const { return m_vWorldSpaceOrigin; }

  /// \brief Returns the matrix used to rotate coordinates from grid space to world space
  const ezMat3& GetRotationToWorldSpace() const { return m_mRotateToWorldspace; }

  /// \brief Returns the matrix used to rotate coordinates from world space to grid space
  const ezMat3& GetRotationToGridSpace() const { return m_mRotateToGridspace; }

  /// \brief Tests where and at which cell the given world space ray intersects the grids bounding box
  bool GetRayIntersection(const ezVec3& vRayStartWorldSpace, const ezVec3& vRayDirNormalizedWorldSpace, float fMaxLength, float& out_fIntersection,
    ezVec2I32& out_vCellCoord) const;

  /// \brief Tests whether a ray would hit the grid bounding box, if it were expanded by a constant.
  bool GetRayIntersectionExpandedBBox(const ezVec3& vRayStartWorldSpace, const ezVec3& vRayDirNormalizedWorldSpace, float fMaxLength,
    float& out_fIntersection, const ezVec3& vExpandBBoxByThis) const;

  void ComputeWorldSpaceCorners(ezVec3* pCorners) const;

  ezResult Serialize(ezStreamWriter& ref_stream) const;
  ezResult Deserialize(ezStreamReader& ref_stream);

private:
  ezUInt16 m_uiGridSizeX;
  ezUInt16 m_uiGridSizeY;

  ezMat3 m_mRotateToWorldspace;
  ezMat3 m_mRotateToGridspace;

  ezVec3 m_vWorldSpaceOrigin;
  ezVec3 m_vLocalSpaceCellSize;
  ezVec3 m_vInverseLocalSpaceCellSize;

  ezDynamicArray<CellData> m_Cells;
};

#include <Utilities/DataStructures/Implementation/GameGrid_inl.h>
