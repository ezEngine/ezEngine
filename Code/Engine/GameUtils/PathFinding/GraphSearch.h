#pragma once

#include <GameUtils/Basics.h>
#include <Foundation/Math/Math.h>
#include <GameUtils/PathFinding/PathState.h>

template<typename PathStateType>
class ezPathSearch
{
public:
  typedef bool (*IsSearchedObjectCallback)(ezInt64 iStartNodeIndex, const PathStateType& StartState);

  struct PathResultData
  {
    EZ_DECLARE_POD_TYPE();

    ezInt64 m_iNodeIndex;
    const PathStateType* m_pPathState;
  };

  void SetPathStateGenerator(ezPathStateGenerator<PathStateType>* pStateGenerator) { m_pStateGenerator = pStateGenerator; }

  ezResult FindPath(ezInt64 iStartNodeIndex, const PathStateType& StartState, ezInt64 iTargetNodeIndex, ezDeque<PathResultData>& out_Path, float fMaxPathCost = ezMath::BasicType<float>::GetInfinity());

  ezResult FindClosest(ezInt64 iStartNodeIndex, const PathStateType& StartState, IsSearchedObjectCallback Callback, ezDeque<PathResultData>& out_Path, float fMaxPathCost = ezMath::BasicType<float>::GetInfinity());

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

