#pragma once

#include <GameEngine/Basics.h>
//#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Containers/Deque.h>
#include <Core/World/Declarations.h>

class ezWorld;

namespace ezWorldUtils
{
  struct EZ_GAMEENGINE_DLL Vertex
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_Position;
    //ezVec2 m_TexCoord;
  };

  struct EZ_GAMEENGINE_DLL Triangle
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_VertexIndices[3];
  };

  struct EZ_GAMEENGINE_DLL Geometry
  {
    ezDeque<Vertex> m_Vertices;
    ezDeque<Triangle> m_Triangles;
  };

  void EZ_GAMEENGINE_DLL ExtractWorldGeometry(const ezWorld& world, Geometry& geo);

  void EZ_GAMEENGINE_DLL ExtractWorldGeometry(const ezWorld& world, Geometry& geo, const ezDeque<ezGameObjectHandle>& selection);

  void EZ_GAMEENGINE_DLL WriteWorldGeometryToOBJ(const Geometry& geo, const char* szFile);
}
