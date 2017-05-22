#pragma once

#include <GameEngine/Basics.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Containers/Deque.h>

struct EZ_GAMEENGINE_DLL ezNavMeshObstacle
{
  ezVec3 m_vPosition;
  ezQuat m_qRotation;

  // TODO ground type etc.
};

struct EZ_GAMEENGINE_DLL ezNavMeshBoxObstacle : public ezNavMeshObstacle
{
  ezVec3 m_vHalfExtents;
};

struct EZ_GAMEENGINE_DLL ezNavMeshDescription
{
  ezNavMeshDescription();
  ~ezNavMeshDescription();

  void Clear();

  ezDeque<ezNavMeshBoxObstacle> m_BoxObstacles;

};

