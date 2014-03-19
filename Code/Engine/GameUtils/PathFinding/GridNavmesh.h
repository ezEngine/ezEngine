#pragma once

#include <GameUtils/DataStructures/GameGrid.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Containers/Deque.h>

/// \brief Takes an ezGameGrid and creates an optimized navmesh structure from it, that is more efficient for path searches.
class EZ_GAMEUTILS_DLL ezGridNavmesh
{
public:
  struct ConvexArea
  {
    EZ_DECLARE_POD_TYPE();

    /// The space that is enclosed by this convex area.
    ezRectU32 m_Rect;

    /// The first AreaEdge that belongs to this ConvexArea.
    ezUInt32 m_uiFirstEdge;

    /// The number of AreaEdge's that belong to this ConvexArea.
    ezUInt32 m_uiNumEdges;
  };

  struct AreaEdge
  {
    EZ_DECLARE_POD_TYPE();

    /// The 'area' of the edge. This is a one cell wide line that is always WITHIN the ConvexArea from where the edge connects to a neighbor area.
    ezRectU16 m_EdgeRect;

    /// The index of the area that can be reached over this edge. This is always a valid index.
    ezInt32 m_iNeighborArea;
  };

  /// \brief Callback that determines whether the cell with index \a uiCell1 and the cell with index \a uiCell2 represent the same type of terrain.
  typedef bool (*CellComparator)(ezUInt32 uiCell1, ezUInt32 uiCell2, void* pPassThrough);

  /// \brief Callback that determines whether the cell with index \a uiCell is blocked entirely (for every type of unit) and therefore can be optimized away.
  typedef bool (*CellBlocked)(ezUInt32 uiCell, void* pPassThrough);

  /// \brief Creates the navmesh from the given ezGameGrid. 
  template<class CellData>
  void CreateFromGrid(const ezGameGrid<CellData>& Grid, CellComparator IsSameCellType, void* pPassThroughSame, CellBlocked IsCellBlocked, void* pPassThroughBlocked);

  /// \brief Returns the index of the ConvexArea at the given cell coordinates. Negative, if the cell is blocked.
  ezInt32 GetAreaAt(const ezVec2I32& Coord) const { return m_NodesGrid.GetCell(Coord); }

  /// \brief Returns the number of convex areas that this navmesh consists of.
  ezUInt32 GetNumConvexAreas() const { return m_ConvexAreas.GetCount(); }

  /// \brief Returns the given convex area by index.
  const ConvexArea& GetConvexArea(ezInt32 iArea) const { return m_ConvexAreas[iArea]; }

  /// \brief Returns the number of edges between convex areas.
  ezUInt32 GetNumAreaEdges() const { return m_GraphEdges.GetCount(); }

  /// \brief Returns the given area edge by index.
  const AreaEdge& GetAreaEdge(ezInt32 iAreaEdge) const { return m_GraphEdges[iAreaEdge]; }

private:
  void UpdateRegion(ezRectU32 region, CellComparator IsSameCellType, void* pPassThrough1, CellBlocked IsCellBlocked, void* pPassThrough2);

  void Optimize(ezRectU32 region, CellComparator IsSameCellType, void* pPassThrough);
  bool OptimizeBoxes(ezRectU32 region, CellComparator IsSameCellType, void* pPassThrough, ezUInt32 uiIntervalX, ezUInt32 uiIntervalY, ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiOffsetX = 0, ezUInt32 uiOffsetY = 0);
  bool CanCreateArea(ezRectU32 region, CellComparator IsSameCellType, void* pPassThrough) const;

  bool CanMergeRight(ezInt32 x, ezInt32 y, CellComparator IsSameCellType, void* pPassThrough, ezRectU32& out_Result) const;
  bool CanMergeDown(ezInt32 x, ezInt32 y, CellComparator IsSameCellType, void* pPassThrough, ezRectU32& out_Result) const;
  bool MergeBestFit(ezRectU32 region, CellComparator IsSameCellType, void* pPassThrough);

  void CreateGraphEdges();
  void CreateGraphEdges(ConvexArea& Area);

  ezRectU32 GetCellBBox(ezInt32 x, ezInt32 y) const;
  void Merge(const ezRectU32& rect);
  void CreateNodes(ezRectU32 region, CellBlocked IsCellBlocked, void* pPassThrough);

  ezGameGrid<ezInt32> m_NodesGrid;
  ezDynamicArray<ConvexArea> m_ConvexAreas;
  ezDeque<AreaEdge> m_GraphEdges;
};

#include <GameUtils/PathFinding/Implementation/GridNavmesh_inl.h>