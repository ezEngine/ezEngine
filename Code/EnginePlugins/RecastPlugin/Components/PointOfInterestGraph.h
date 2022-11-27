#pragma once

#include <Foundation/Math/Vec3.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>
#include <Utilities/DataStructures/DynamicOctree.h>

template <typename POINTTYPE>
class ezPointOfInterestGraph
{
public:
  void Initialize(const ezVec3& vCenter, const ezVec3& vHalfExtents, float fCellSize = 1.0f);

  POINTTYPE& AddPoint(const ezVec3& vPosition);

  void FindPointsOfInterest(const ezVec3& vPosition, float fRadius, ezDynamicArray<ezUInt32>& out_points) const;

  const ezDeque<POINTTYPE>& GetPoints() const { return m_Points; }
  ezDeque<POINTTYPE>& AccessPoints() { return m_Points; }

private:
  ezDeque<POINTTYPE> m_Points;
  ezDynamicOctree m_Octree;
};

#include <RecastPlugin/Components/PointOfInterestGraph_inl.h>
