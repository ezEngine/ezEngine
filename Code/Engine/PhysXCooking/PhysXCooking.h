#pragma once

#include <PhysXCooking/Basics.h>
#include <PhysXPlugin/PluginInterface.h>

using namespace physx;

class EZ_PHYSXCOOKING_DLL ezPhysXCooking
{
public:

  static void Startup();
  static void Shutdown();

  struct Mesh
  {
    bool m_bFlipNormals;
    ezDynamicArray<ezVec3> m_Vertices;
    ezDynamicArray<ezUInt8> m_VerticesInPolygon;
    ezDynamicArray<ezUInt32> m_PolygonIndices;
  };

  static ezResult CookTriangleMesh(const Mesh& mesh, ezStreamWriter& OutputStream);


private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(PhysX, PhysXCooking);

  static PxCooking* s_pCooking;
  static ezPhysXInterface* s_pPhysX;
};

