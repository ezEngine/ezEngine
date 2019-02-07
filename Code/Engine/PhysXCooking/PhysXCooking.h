#pragma once

#include <PhysXCooking/PhysXCookingDLL.h>
#include <PhysXPlugin/PhysXInterface.h>
#include <Foundation/Types/Status.h>

using namespace physx;

namespace physx
{
  class PxSimpleTriangleMesh;
}

class ezStreamWriter;
class ezChunkStreamWriter;

struct EZ_PHYSXCOOKING_DLL ezPhysXCookingMesh
{
  bool m_bFlipNormals = false;
  ezDynamicArray<ezVec3> m_Vertices;
  ezDynamicArray<ezUInt8> m_VerticesInPolygon;
  ezDynamicArray<ezUInt32> m_PolygonIndices;
  ezDynamicArray<ezUInt16> m_PolygonSurfaceID;
};

class EZ_PHYSXCOOKING_DLL ezPhysXCooking
{
public:

  static void Startup();
  static void Shutdown();

  static ezResult CookTriangleMesh(const ezPhysXCookingMesh& mesh, ezStreamWriter& OutputStream);
  static ezResult CookConvexMesh(const ezPhysXCookingMesh& mesh, ezStreamWriter& OutputStream);
  static ezResult ComputeConvexHull(const ezPhysXCookingMesh& mesh, ezPhysXCookingMesh& outMesh);
  static ezStatus WriteResourceToStream(ezChunkStreamWriter& stream, const ezPhysXCookingMesh& mesh, const ezArrayPtr<ezString>& surfaces, bool bConvexMesh);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(PhysX, PhysXCooking);

  static void CreateMeshDesc(const ezPhysXCookingMesh& mesh, physx::PxSimpleTriangleMesh& desc, ezDynamicArray<ezUInt32>& TriangleIndices);

  static PxCooking* s_pCooking;
  static ezPhysXInterface* s_pPhysX;
};

