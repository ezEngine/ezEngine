#pragma once

#include <GameUtils/Basics.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Containers/HashTable.h>

struct ezPathStateBase
{
  EZ_DECLARE_POD_TYPE();

  ezPathStateBase()
  {
    m_iReachedThroughNode = 0;
    m_fCostToNode = 0.0f;
    m_fEstimatedCostToTarget = 0.0f;
  }

  /// Initialized by the path searcher
  ezInt64 m_iReachedThroughNode;

  /// Each ezPathStateGenerator needs to update the costs by taking the nodes from the predecessor state and adding a non-zero cost value to it
  float m_fCostToNode;

  /// To get directed path searches (A*) this estimation needs to be filled out. It must be the sum of m_fCostToNode and an estimation
  /// of how much the remaining path will cost (at least). The estimation must be 'optimistic', ie. it musn't be possible to reach
  /// the destination with less costs than what was estimated.
  float m_fEstimatedCostToTarget;
};

template<typename PathStateType>
class ezPathSearch;

template<typename PathStateType>
class ezPathStateGenerator
{
public:
  virtual void GenerateAdjacentStates(ezInt64 iNodeIndex, const PathStateType& StartState, ezPathSearch<PathStateType>* pPathSearch) = 0;

  virtual void StartSearch(ezInt64 iStartNodeIndex, const PathStateType* pStartState, ezInt64 iTargetNodeIndex) { }

  virtual void SearchFinished(ezResult res) { }
};

#include <GameUtils/PathFinding/Implementation/PathState_inl.h>

