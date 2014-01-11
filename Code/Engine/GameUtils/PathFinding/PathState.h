#pragma once

#include <GameUtils/Basics.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Containers/HashTable.h>

struct ezPathStateBase
{
  EZ_DECLARE_POD_TYPE();

  ezInt64 m_iReachedThroughNode;
  float m_fCostToNode;
  float m_fEstimatedCostToTarget;

};

template<typename PathStateType>
class ezPathSearch;

template<typename PathStateType>
class ezPathStateGenerator
{
public:
  virtual void GenerateAdjacentStates(ezInt64 iNodeIndex, const PathStateType& StartState, ezPathSearch<PathStateType>* pPathSearch) = 0;


};

template<typename PathStateType>
class ezPathSearch
{
public:

  void SetPathStateGenerator(ezPathStateGenerator<PathStateType>* pStateGenerator) { m_pStateGenerator = pStateGenerator; }

  ezResult FindPath(ezInt64 iStartNodeIndex, const PathStateType& StartState, ezInt64 iTargetNodeIndex, ezDeque<ezInt64>& out_Path, float fMaxPathCost = ezMath::BasicType<float>::GetInfinity());

  void AddPathNode(ezInt64 iNodeIndex, const PathStateType& NewState);

private:
  void ClearPathStates();
  ezInt64 FindBestNodeToExpand(PathStateType*& out_pPathState);
  void FillOutPathResult(ezInt64 iEndNodeIndex, ezDeque<ezInt64>& out_Path);

  ezPathStateGenerator<PathStateType>* m_pStateGenerator;

  ezHashTable<ezInt64, PathStateType> m_PathStates;

  ezDeque<ezInt64> m_StateQueue;
};

#include <GameUtils/PathFinding/Implementation/PathState_inl.h>

