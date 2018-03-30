#pragma once

#include <GameEngine/AI/PointOfInterestGraph.h>

template<typename POINTTYPE>
void ezPointOfInterestGraph<POINTTYPE>::Initialize(const ezVec3& center, const ezVec3& halfExtents, float cellSize)
{
  m_Points.Clear();
  m_Octree.CreateTree(center, halfExtents, cellSize);

}

template<typename POINTTYPE>
POINTTYPE& ezPointOfInterestGraph<POINTTYPE>::AddPoint(const ezVec3& position)
{
  const ezUInt32 id = m_Points.GetCount();
  auto& pt = m_Points.ExpandAndGetRef();

  m_Octree.InsertObject(position, ezVec3::ZeroVector(), 0, id, nullptr, true);

  return pt;
}

template<typename POINTTYPE>
void ezPointOfInterestGraph<POINTTYPE>::FindPointsOfInterest(const ezVec3& position, float radius, ezDynamicArray<ezUInt32>& out_Points) const
{
  if (m_Octree.IsEmpty())
    return;

  struct Data
  {
    ezDynamicArray<ezUInt32>* m_pResults;
  };

  Data data;
  data.m_pResults = &out_Points;

  auto cb = [](void* pPassThrough, ezDynamicTreeObjectConst Object) -> bool
  {
    auto pData = static_cast<Data*>(pPassThrough);

    const ezUInt32 id = (ezUInt32)Object.Value().m_iObjectInstance;
    pData->m_pResults->PushBack(id);

    return true;
  };

  m_Octree.FindObjectsInRange(position, radius, cb, &data);
}

