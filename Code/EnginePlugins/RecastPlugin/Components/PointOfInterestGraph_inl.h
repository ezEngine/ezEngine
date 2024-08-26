#pragma once

#include <RecastPlugin/Components/PointOfInterestGraph.h>

template <typename POINTTYPE>
void ezPointOfInterestGraph<POINTTYPE>::Initialize(const ezVec3& vCenter, const ezVec3& vHalfExtents, float fCellSize)
{
  m_Points.Clear();
  m_Octree.CreateTree(vCenter, vHalfExtents, fCellSize);
}

template <typename POINTTYPE>
POINTTYPE& ezPointOfInterestGraph<POINTTYPE>::AddPoint(const ezVec3& vPosition)
{
  const ezUInt32 id = m_Points.GetCount();
  auto& pt = m_Points.ExpandAndGetRef();

  m_Octree.InsertObject(vPosition, ezVec3::MakeZero(), 0, id, nullptr, true).IgnoreResult();

  return pt;
}

template <typename POINTTYPE>
void ezPointOfInterestGraph<POINTTYPE>::FindPointsOfInterest(const ezVec3& vPosition, float fRadius, ezDynamicArray<ezUInt32>& out_points) const
{
  if (m_Octree.IsEmpty())
    return;

  struct Data
  {
    ezDynamicArray<ezUInt32>* m_pResults;
  };

  Data data;
  data.m_pResults = &out_points;

  auto cb = [](void* pPassThrough, ezDynamicTreeObjectConst object) -> bool
  {
    auto pData = static_cast<Data*>(pPassThrough);

    const ezUInt32 id = (ezUInt32)object.Value().m_iObjectInstance;
    pData->m_pResults->PushBack(id);

    return true;
  };

  m_Octree.FindObjectsInRange(vPosition, fRadius, cb, &data);
}
