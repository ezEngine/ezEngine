#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Math.h>
#include <Utilities/UtilitiesDLL.h>

/// \brief Base class for all path finding state objects.
///
/// Path states carry information about the current state of a pathfinding agent at a specific node.
/// This includes costs accumulated so far and can be extended to include custom state like facing direction,
/// remaining fuel, unlocked abilities, or any other data that affects movement possibilities.
struct ezPathState
{
  EZ_DECLARE_POD_TYPE();

  ezPathState()
  {
    m_iReachedThroughNode = 0;
    m_fCostToNode = 0.0f;
    m_fEstimatedCostToTarget = 0.0f;
  }

  /// Back-pointer to the node from which this node was reached.
  ///
  /// Set automatically by the path searcher during the search process.
  /// Used to reconstruct the final path once the target is found.
  ezInt64 m_iReachedThroughNode;

  /// The accumulated cost to reach this node from the start.
  ///
  /// Must be updated by ezPathStateGenerator implementations by taking the cost
  /// from the predecessor state and adding the movement cost to reach this node.
  /// Should always be non-negative and increase along the path.
  float m_fCostToNode;

  /// To get directed path searches (A*) this estimation needs to be filled out. It must be the sum of m_fCostToNode and an estimation
  /// of how much the remaining path will cost (at least).
  /// To always get the 'optimal path' (the shortest) the estimation should be 'optimistic', ie. it mustn't be possible to reach the
  /// destination with less costs than what was estimated.
  /// However, the estimation can also be 'pessimistic', ie. the final path will actually cost less than what was estimated. In this case
  /// path searches can be a lot faster, but they will also produce paths that are longer than necessary and might be overly winding.
  float m_fEstimatedCostToTarget;
};

template <typename PathStateType>
class ezPathSearch;

/// \brief Base class for path state generators that define how pathfinding expands from one node to adjacent nodes.
///
/// A path state generator is a class that takes one 'path state' (typically a node in a graph) and generates all the adjacent nodes
/// that can be reached from there. It carries state, which allows to expand nodes only in certain directions, depending on what actions
/// are possible at some graph node.
///
/// This can be used to do path searches where the capabilities of a unit are taken into account, such as limited turning speeds.
/// The 'state' that is then carried is the current orientation of the unit at that point along the path, which determines into which
/// directions the path search can be expanded.
///
/// PathStateType needs to be derived from ezPathState.
template <typename PathStateType>
class ezPathStateGenerator
{
public:
  /// \brief Generates all valid adjacent states from the current node and state.
  ///
  /// This is the core expansion function called during pathfinding. For each valid adjacent node,
  /// create a new PathStateType with updated costs and state, then add it to the search via
  /// pPathSearch->AddPathNode().
  ///
  /// \param iNodeIndex The current node being expanded (e.g., grid cell index, navmesh triangle ID)
  /// \param StartState The current path state at this node (costs, direction, resources, etc.)
  /// \param pPathSearch The search object to add discovered adjacent nodes to
  ///
  /// Example for a simple grid:
  /// ```cpp
  /// for (auto& neighbor : GetGridNeighbors(iNodeIndex))
  /// {
  ///   PathStateType newState = StartState;
  ///   newState.m_fCostToNode += GetMovementCost(iNodeIndex, neighbor.index);
  ///   newState.m_fEstimatedCostToTarget = newState.m_fCostToNode + GetHeuristic(neighbor.index, targetIndex);
  ///   pPathSearch->AddPathNode(neighbor.index, newState);
  /// }
  /// ```
  virtual void GenerateAdjacentStates(ezInt64 iNodeIndex, const PathStateType& StartState, ezPathSearch<PathStateType>* pPathSearch) = 0;

  /// \brief Automatically called by ezPathSearch objects when a new path search is about to start (ezPathSearch::FindClosest).
  /// Allows the generator to do some initial setup.
  virtual void StartSearchForClosest(ezInt64 iStartNodeIndex, const PathStateType* pStartState) {}

  /// \brief Automatically called by ezPathSearch objects when a new path search is about to start (ezPathSearch::FindPath).
  /// Allows the generator to do some initial setup.
  virtual void StartSearch(ezInt64 iStartNodeIndex, const PathStateType* pStartState, ezInt64 iTargetNodeIndex) {}

  /// \brief Automatically called by ezPathSearch objects when a path search was finished.
  /// Allows the generator to do some cleanup.
  virtual void SearchFinished(ezResult res) {}
};
