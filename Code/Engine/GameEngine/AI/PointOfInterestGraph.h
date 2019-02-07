#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <Foundation/Math/Vec3.h>
#include <Utilities/DataStructures/DynamicOctree.h>

template<typename POINTTYPE>
class ezPointOfInterestGraph
{
public:
  void Initialize(const ezVec3& center, const ezVec3& halfExtents, float cellSize = 1.0f);

  POINTTYPE& AddPoint(const ezVec3& position);

  void FindPointsOfInterest(const ezVec3& position, float radius, ezDynamicArray<ezUInt32>& out_Points) const;

  const ezDeque<POINTTYPE>& GetPoints() const { return m_Points; }
  ezDeque<POINTTYPE>& AccessPoints() { return m_Points; }

private:
  ezDeque<POINTTYPE> m_Points;
  ezDynamicOctree m_Octree;
};

#include <GameEngine/AI/Implementation/PointOfInterestGraph_inl.h>

