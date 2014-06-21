#pragma once

#include <GameUtils/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Mat3.h>

/// \brief ezGameGrid is a general purpose 2D grid structure that has several convenience functions which are often required when working with a grid.
template<class CellData>
class ezGameGrid
{
public:
  enum Orientation
  {
    InPlaneXY,  ///< The grid is expected to lie in the XY plane in worldspace (when Y is up, this is similar to a 2D side scroller)
    InPlaneXZ,  ///< The grid is expected to lie in the XZ plane in worldspace (when Y is up, this is similar to a top down RTS game)
    InPlaneXminusZ,  ///< The grid is expected to lie in the XZ plane in worldspace (when Y is up, this is similar to a top down RTS game)
  };

  ezGameGrid();

  /// \brief Clears all data and reallocates the grid with the given dimensions.
  void CreateGrid(ezUInt16 uiWidth, ezUInt16 uiHeight);

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

  /// \brief Returns the worldspace size of each cell.
  ezVec3 GetWorldSpaceCellSize() const { return m_vWorldSpaceCellSize; }

  /// \brief Returns the coordinate of the cell at the given world-space position. The world space dimension must be set for this to work.
  /// The indices might be outside valid ranges (negative, larger than the maximum size).
  ezVec2I32 GetCellAtWorldPosition(const ezVec3& vWorldSpacePos) const;

  /// \brief Returns the number of cells along the X axis.
  ezUInt16 GetGridWidth() const  { return m_uiWidth;  }

  /// \brief Returns the number of cells along the Y axis.
  ezUInt16 GetGridHeight() const { return m_uiHeight;  }
  
  /// \brief Returns the world-space bounding box of the grid, as specified via SetWorldDimensions.
  ezBoundingBox GetWorldBoundingBox() const;

  /// \brief Returns the total number of cells.
  ezUInt32 GetNumCells() const { return m_uiWidth * m_uiHeight; }

  /// \brief Gives access to a cell by cell index.
  CellData& GetCell(ezUInt32 uiIndex) { return m_Cells[uiIndex]; }

  /// \brief Gives access to a cell by cell index.
  const CellData& GetCell(ezUInt32 uiIndex) const { return m_Cells[uiIndex]; }

  /// \brief Gives access to a cell by cell coordinates.
  CellData& GetCell(const ezVec2I32& Coord) { return m_Cells[ConvertCellCoordinateToIndex(Coord)]; }

  /// \brief Gives access to a cell by cell coordinates.
  const CellData& GetCell(const ezVec2I32& Coord) const { return m_Cells[ConvertCellCoordinateToIndex(Coord)]; }

  /// \brief Converts a cell index into a 2D cell coordinate.
  ezVec2I32 ConvertCellIndexToCoordinate(ezUInt32 uiIndex) const { return ezVec2I32 (uiIndex % m_uiWidth, uiIndex / m_uiWidth); }

  /// \brief Converts a cell coordinate into a cell index.
  ezUInt32 ConvertCellCoordinateToIndex(const ezVec2I32& Coord) const { return Coord.y * m_uiWidth + Coord.x; }

  /// \brief Returns the lower left world space position of the cell with the given coordinates.
  ezVec3 GetCellWorldSpaceOrigin(const ezVec2I32& Coord) const;

  /// \brief Returns the center world space position of the cell with the given coordinates.
  ezVec3 GetCellWorldSpaceCenter(const ezVec2I32& Coord) const;

  /// \brief Checks whether the given cell coordinate is inside valid ranges.
  bool IsValidCellCoordinate(const ezVec2I32& Coord) const;

  /// \brief Casts a world space ray through the grid and determines which cell is hit (if any).
  bool PickCell(const ezVec3& vRayStartPos, const ezVec3& vRayDirNorm, ezVec2I32* out_CellCoord, ezVec3* out_vIntersection = nullptr) const;

private:
  ezUInt16 m_uiWidth;
  ezUInt16 m_uiHeight;

  ezMat3 m_RotateToWorldspace;
  ezMat3 m_RotateToGridspace;

  ezVec3 m_vWorldSpaceOrigin;
  ezVec3 m_vWorldSpaceCellSize;
  ezVec3 m_vInverseWorldSpaceCellSize;

  ezDynamicArray<CellData> m_Cells;
};

#include <GameUtils/DataStructures/Implementation/GameGrid_inl.h>

