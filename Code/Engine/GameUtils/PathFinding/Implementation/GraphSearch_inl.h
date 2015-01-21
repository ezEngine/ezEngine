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
  out_pPathState = nullptr;
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

  EZ_ASSERT_DEV(out_pPathState != nullptr, "Implementation Error");

  const ezInt64 iBestNodeIndex = m_StateQueue[iBestInQueue];
  m_StateQueue.RemoveAtSwap(iBestInQueue);

  return iBestNodeIndex;
}

template<typename PathStateType>
void ezPathSearch<PathStateType>::FillOutPathResult(ezInt64 iEndNodeIndex, ezDeque<PathResultData>& out_Path)
{
  out_Path.Clear();

  while (true)
  {
    const PathStateType* pCurState = &m_PathStates[iEndNodeIndex];

    PathResultData r;
    r.m_iNodeIndex = iEndNodeIndex;
    r.m_pPathState = pCurState;

    out_Path.PushFront(r);

    if (iEndNodeIndex == pCurState->m_iReachedThroughNode)
      return;

    iEndNodeIndex = pCurState->m_iReachedThroughNode;
  }
}

template<typename PathStateType>
void ezPathSearch<PathStateType>::AddPathNode(ezInt64 iNodeIndex, const PathStateType& NewState)
{
  EZ_ASSERT_DEV(NewState.m_fCostToNode            >   m_CurState.m_fCostToNode, "The costs must grow from one node to the next.\nStart Node Costs: %.2f\nAdjacent Node Costs: %.2f", m_CurState.m_fCostToNode, NewState.m_fCostToNode);
  //EZ_ASSERT_DEV(NewState.m_fEstimatedCostToTarget >=  m_CurState.m_fEstimatedCostToTarget, "The estimated path costs cannot go down, the heuristic must be 'optimistic' regarding to the real costs.\nEstimated Costs from Current: %.2f\nEstimated Costs from Adjacent: %.2f", m_pCurPathState->m_fEstimatedCostToTarget, NewState.m_fEstimatedCostToTarget);
  EZ_ASSERT_DEV(NewState.m_fEstimatedCostToTarget >=  NewState.m_fCostToNode, "Unrealistic expectations will get you nowhere.");

  PathStateType* pExistingState;

  if (m_PathStates.TryGetValue(iNodeIndex, pExistingState))
  {
    // state already exists in the hash table, and has a lower cost -> ignore the new state
    if (pExistingState->m_fCostToNode <= NewState.m_fCostToNode)
      return;

    // incoming state is better than the existing state -> update existing state
    *pExistingState = NewState;
    pExistingState->m_iReachedThroughNode = m_iCurNodeIndex;
    return;
  }

  // the state has not been reached before -> insert it
  pExistingState = &m_PathStates[iNodeIndex];

  *pExistingState = NewState;
  pExistingState->m_iReachedThroughNode = m_iCurNodeIndex;

  // put it into the queue of states that still need to be expanded
  m_StateQueue.PushBack(iNodeIndex);
}

template<typename PathStateType>
ezResult ezPathSearch<PathStateType>::FindPath(ezInt64 iStartNodeIndex, const PathStateType& StartState, ezInt64 iTargetNodeIndex, ezDeque<PathResultData>& out_Path, float fMaxPathCost /* = Infinity */)
{
  EZ_ASSERT_DEV(m_pStateGenerator != nullptr, "No Path State Generator is set.");

  ClearPathStates();

  if (iStartNodeIndex == iTargetNodeIndex)
  {
    m_PathStates.Reserve(1);

    m_PathStates[iTargetNodeIndex] = StartState;

    PathResultData r;
    r.m_iNodeIndex = iTargetNodeIndex;
    r.m_pPathState = &m_PathStates[iTargetNodeIndex];

    out_Path.Clear();
    out_Path.PushBack(r);

    return EZ_SUCCESS;
  }

  m_PathStates.Reserve(10000);

  PathStateType& FirstState = m_PathStates[iStartNodeIndex];

  m_pStateGenerator->StartSearch(iStartNodeIndex, &FirstState, iTargetNodeIndex);

  // make sure the first state references itself, as that is a termination criterion
  FirstState = StartState;
  FirstState.m_iReachedThroughNode = iStartNodeIndex;

  // put the start state into the to-be-expanded queue
  m_StateQueue.PushBack(iStartNodeIndex);

  // while the queue is not empty, expand the next node and see where that gets us
  while (!m_StateQueue.IsEmpty())
  {
    PathStateType* pCurState;
    m_iCurNodeIndex = FindBestNodeToExpand(pCurState);

    // we have reached the target node, generate the final path result
    if (m_iCurNodeIndex == iTargetNodeIndex)
    {
      FillOutPathResult(m_iCurNodeIndex, out_Path);
      m_pStateGenerator->SearchFinished(EZ_SUCCESS);
      return EZ_SUCCESS;
    }

    // The heuristic may overestimate how much it takes to reach the destination
    // thus even though the heuristic tells us we may not be able to make it, we cannot rely on that, but need to look at
    // the actual costs
    if (pCurState->m_fCostToNode >= fMaxPathCost)
    {
      m_pStateGenerator->SearchFinished(EZ_FAILURE);
      return EZ_FAILURE;
    }

    m_CurState = *pCurState;

    // let the generate append all the nodes that we can reach from here
    m_pStateGenerator->GenerateAdjacentStates(m_iCurNodeIndex, m_CurState, this);
  }

  m_pStateGenerator->SearchFinished(EZ_FAILURE);
  return EZ_FAILURE;
}


template<typename PathStateType>
ezResult ezPathSearch<PathStateType>::FindClosest(ezInt64 iStartNodeIndex, const PathStateType& StartState, IsSearchedObjectCallback Callback, ezDeque<PathResultData>& out_Path, float fMaxPathCost)
{
  EZ_ASSERT_DEV(m_pStateGenerator != nullptr, "No Path State Generator is set.");

  ClearPathStates();

  m_PathStates.Reserve(10000);

  PathStateType& FirstState = m_PathStates[iStartNodeIndex];

  m_pStateGenerator->StartSearchForClosest(iStartNodeIndex, &FirstState);

  // make sure the first state references itself, as that is a termination criterion
  FirstState = StartState;
  FirstState.m_iReachedThroughNode = iStartNodeIndex;

  // put the start state into the to-be-expanded queue
  m_StateQueue.PushBack(iStartNodeIndex);

  // while the queue is not empty, expand the next node and see where that gets us
  while (!m_StateQueue.IsEmpty())
  {
    PathStateType* pCurState;
    m_iCurNodeIndex = FindBestNodeToExpand(pCurState);

    // we have reached the target node, generate the final path result
    if (Callback(m_iCurNodeIndex, *pCurState))
    {
      FillOutPathResult(m_iCurNodeIndex, out_Path);
      m_pStateGenerator->SearchFinished(EZ_SUCCESS);
      return EZ_SUCCESS;
    }

    // The heuristic may overestimate how much it takes to reach the destination
    // thus even though the heuristic tells us we may not be able to make it, we cannot rely on that, but need to look at
    // the actual costs
    if (pCurState->m_fCostToNode >= fMaxPathCost)
    {
      m_pStateGenerator->SearchFinished(EZ_FAILURE);
      return EZ_FAILURE;
    }

    m_CurState = *pCurState;

    // let the generate append all the nodes that we can reach from here
    m_pStateGenerator->GenerateAdjacentStates(m_iCurNodeIndex, m_CurState, this);
  }

  m_pStateGenerator->SearchFinished(EZ_FAILURE);
  return EZ_FAILURE;
}

