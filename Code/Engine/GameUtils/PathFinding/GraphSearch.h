#pragma once

#include <GameUtils/Basics.h>
#include <Foundation/Math/Math.h>
#include <GameUtils/PathFinding/PathState.h>

/// \brief Implements a directed breadth-first search through a graph (A*).
///
/// You can search for a path to a specific location using FindPath() or to the closest node that fulfills some arbitrary criteria
/// using FindClosest().
///
/// PathStateType must be derived from ezPathStateBase and can be used for keeping track of certain state along a path and to modify
/// the path search dynamically.
template<typename PathStateType>
class ezPathSearch
{
public:

  /// \brief Used by FindClosest() to query whether the currently visited node fulfills the termination criteria.
  typedef bool (*IsSearchedObjectCallback)(ezInt64 iStartNodeIndex, const PathStateType& StartState);

  /// \brief FindPath() and FindClosest() return an array of these objects as the path result.
  struct PathResultData
  {
    EZ_DECLARE_POD_TYPE();

    /// \brief The index of the node that was visited.
    ezInt64 m_iNodeIndex;

    /// \brief Pointer to the path state that was active at that step along the path.
    const PathStateType* m_pPathState;
  };

  /// \brief Sets the ezPathStateGenerator that should be used by this ezPathSearch object.
  void SetPathStateGenerator(ezPathStateGenerator<PathStateType>* pStateGenerator) { m_pStateGenerator = pStateGenerator; }

  /// \brief Searches for a path that starts at the graph node \a iStartNodeIndex with the start state \a StartState and shall terminate when
  /// the graph node \a iTargetNodeIndex was reached.
  ///
  /// Returns EZ_FAILURE if no path could be found.
  /// Returns the path result as a list of PathResultData objects in \a out_Path.
  ///
  /// The path search is stopped (and thus fails) if the path reaches costs of \a fMaxPathCost or higher.
  ezResult FindPath(ezInt64 iStartNodeIndex, const PathStateType& StartState, ezInt64 iTargetNodeIndex, ezDeque<PathResultData>& out_Path, float fMaxPathCost = ezMath::BasicType<float>::GetInfinity());

  /// \brief Searches for a path that starts at the graph node \a iStartNodeIndex with the start state \a StartState and shall terminate when
  /// a graph node is reached for which \a Callback return true.
  ///
  /// Returns EZ_FAILURE if no path could be found.
  /// Returns the path result as a list of PathResultData objects in \a out_Path.
  ///
  /// The path search is stopped (and thus fails) if the path reaches costs of \a fMaxPathCost or higher.
  ezResult FindClosest(ezInt64 iStartNodeIndex, const PathStateType& StartState, IsSearchedObjectCallback Callback, ezDeque<PathResultData>& out_Path, float fMaxPathCost = ezMath::BasicType<float>::GetInfinity());

  /// \brief Needs to be called by the used ezPathStateGenerator to add nodes to evaluate.
  void AddPathNode(ezInt64 iNodeIndex, const PathStateType& NewState);

private:
  void ClearPathStates();
  ezInt64 FindBestNodeToExpand(PathStateType*& out_pPathState);
  void FillOutPathResult(ezInt64 iEndNodeIndex, ezDeque<PathResultData>& out_Path);

  ezPathStateGenerator<PathStateType>* m_pStateGenerator;

  ezHashTable<ezInt64, PathStateType> m_PathStates;

  ezDeque<ezInt64> m_StateQueue;

  ezInt64 m_iCurNodeIndex;
  PathStateType m_CurState;
};






#include <GameUtils/PathFinding/Implementation/GraphSearch_inl.h>

