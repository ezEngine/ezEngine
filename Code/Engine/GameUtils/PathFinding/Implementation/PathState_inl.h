#pragma once

template<typename PathStateType>
void ezPathSearch<PathStateType>::ClearPathStates()
{
  m_PathStates.Clear();
  m_StateQueue.Clear();
}

template<typename PathStateType>
ezInt64 ezPathSearch<PathStateType>::FindBestNodeToExpand(PathStateType*& out_pPathState)
{
  float fLowestEstimation = ezMath::BasicType<float>::GetInfinity();
  out_pPathState = NULL;
  ezUInt32 iBestInQueue = 0;

  for (ezUInt32 i = 0; i < m_StateQueue.GetCount(); ++i)
  {
    const ezInt64 iNodeIndex = m_StateQueue[i];

    PathStateType* pState = &m_PathStates[iNodeIndex];

    if (pState->m_fEstimatedCostToTarget < fLowestEstimation)
    {
      fLowestEstimation = pState->m_fEstimatedCostToTarget;
      out_pPathState = pState;
      iBestInQueue = i;
    }
  }

  EZ_ASSERT(out_pPathState != NULL, "Implementation Error");

  const ezInt64 iBestNodeIndex = m_StateQueue[iBestInQueue];
  m_StateQueue.RemoveAtSwap(iBestInQueue);

  return iBestNodeIndex;
}

template<typename PathStateType>
void ezPathSearch<PathStateType>::FillOutPathResult(ezInt64 iEndNodeIndex, ezDeque<ezInt64>& out_Path)
{
  out_Path.Clear();

  while (true)
  {
    out_Path.PushFront(iEndNodeIndex);

    const PathStateType* pCurState = &m_PathStates[iEndNodeIndex];

    if (iEndNodeIndex == pCurState->m_iReachedThroughNode)
      return;

    iEndNodeIndex = pCurState->m_iReachedThroughNode;
  }
}

template<typename PathStateType>
ezResult ezPathSearch<PathStateType>::FindPath(ezInt64 iStartNodeIndex, const PathStateType& StartState, ezInt64 iTargetNodeIndex, ezDeque<ezInt64>& out_Path, float fMaxPathCost /* = Infinity */)
{
  ClearPathStates();

  PathStateType& FirstState = m_PathStates[iStartNodeIndex];

  // make sure the first state references itself, as that is a termination criterion
  FirstState = StartState;
  FirstState.m_iReachedThroughNode = iStartNodeIndex;

  // put the start state into the to-be-expanded queue
  m_StateQueue.PushBack(iStartNodeIndex);

  // while the queue is not empty, expand the next node and see where that gets us
  while (!m_StateQueue.IsEmpty())
  {
    PathStateType* pCurState;
    ezInt64 iCurNodeIndex = FindBestNodeToExpand(pCurState);

    // we have reached the target node, generate the final path result
    if (iCurNodeIndex == iTargetNodeIndex)
    {
      FillOutPathResult(iCurNodeIndex, out_Path);
      return EZ_SUCCESS;
    }

    // The heuristic must give a lower bound to what is required to reach the target
    // That means once our heuristic says we can't reach the target within the maximum path costs
    // we can just stop the search, as there is no other possible path that might be shorter (we just picked the shortest estimate above)
    if (pCurState->m_fEstimatedCostToTarget >= fMaxPathCost)
      return EZ_FAILURE;

    // let the generate append all the nodes that we can reach from here
    m_pStateGenerator->GenerateAdjacentStates(iCurNodeIndex, *pCurState, this);
  }

  return EZ_FAILURE;
}

template<typename PathStateType>
void ezPathSearch<PathStateType>::AddPathNode(ezInt64 iNodeIndex, const PathStateType& NewState)
{
  PathStateType* pExistingState;

  if (m_PathStates.TryGetValue(iNodeIndex, pExistingState))
  {
    // state already exists in the hash table, and has a lower cost -> ignore the new state
    if (pExistingState->m_fCostToNode <= NewState.m_fCostToNode)
      return;

    // incoming state is better than the existing state -> update existing state
    *pExistingState = NewState;
    return;
  }

  // the state has not been reached before -> insert it
  m_PathStates[iNodeIndex] = NewState;

  // put it into the queue of states that still need to be expanded
  m_StateQueue.PushBack(iNodeIndex);
}

