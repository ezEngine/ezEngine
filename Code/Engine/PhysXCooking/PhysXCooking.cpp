#include <PhysXCooking/PCH.h>
#include <PhysXCooking/PhysXCooking.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Configuration/AbstractInterfaceRegistry.h>
#include <PxPhysicsAPI.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(PhysX, PhysXCooking)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "PhysXPlugin"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezPhysXCooking::Startup();
  }

  ON_CORE_SHUTDOWN
  {
    ezPhysXCooking::Shutdown();
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION


PxCooking* ezPhysXCooking::s_pCooking = nullptr;
ezPhysXInterface* ezPhysXCooking::s_pPhysX = nullptr;

void ezPhysXCooking::Startup()
{
  s_pPhysX = ezAbstractInterfaceRegistry::RetrieveImplementationForInterface<ezPhysXInterface>("ezPhysXInterface");

  PxCookingParams params = PxCookingParams(s_pPhysX->GetPhysXAPI()->getTolerancesScale());
  params.targetPlatform = PxPlatform::ePC;

  s_pCooking = PxCreateCooking(PX_PHYSICS_VERSION, s_pPhysX->GetPhysXAPI()->getFoundation(), params);
  EZ_ASSERT_DEV(s_pCooking != nullptr, "Initializing PhysX cooking API failed");
}

void ezPhysXCooking::Shutdown()
{
  if (s_pCooking)
  {
    s_pCooking->release();
    s_pCooking = nullptr;
  }
}

class ezPxOutStream : public PxOutputStream
{
public:
  ezPxOutStream(ezStreamWriter* pStream) : m_pStream(pStream) {}

  virtual PxU32 write(const void* src, PxU32 count) override
  {
    if (m_pStream->WriteBytes(src, count).Succeeded())
      return count;

    return 0;
  }

  ezStreamWriter* m_pStream;
};

ezResult ezPhysXCooking::CookTriangleMesh(const Mesh& mesh, ezStreamWriter& OutputStream)
{
  PxTriangleMeshDesc desc;
  desc.setToDefault();

  desc.points.count = mesh.m_Vertices.GetCount();
  desc.points.stride = sizeof(ezVec3);
  desc.points.data = mesh.m_Vertices.GetData();

  ezUInt32 uiTriangles = 0;
  for (auto numIndices : mesh.m_VerticesInPolygon)
    uiTriangles += numIndices - 2;

  ezDynamicArray<ezUInt32> TriangleIndices;
  TriangleIndices.SetCount(uiTriangles * 3);

  ezUInt32 uiFirstIndex = 0;
  ezUInt32 uiFirstTriangleIdx = 0;
  for (auto numIndices : mesh.m_VerticesInPolygon)
  {
    for (ezUInt32 t = 2; t < numIndices; ++t)
    {
      TriangleIndices[uiFirstTriangleIdx + 0] = mesh.m_PolygonIndices[uiFirstIndex];
      TriangleIndices[uiFirstTriangleIdx + 1] = mesh.m_PolygonIndices[uiFirstIndex + t - 1];
      TriangleIndices[uiFirstTriangleIdx + 2] = mesh.m_PolygonIndices[uiFirstIndex + t];

      uiFirstTriangleIdx += 3;
    }

    uiFirstIndex += numIndices;
  }

  desc.triangles.count = uiTriangles;
  desc.triangles.stride = sizeof(ezUInt32) * 3;
  desc.triangles.data = TriangleIndices.GetData();

  desc.flags.set(mesh.m_bFlipNormals ? PxMeshFlag::eFLIPNORMALS : (PxMeshFlag::Enum)0);

  EZ_ASSERT_DEV(desc.isValid(), "PhysX PxTriangleMeshDesc is invalid");

  ezPxOutStream PassThroughStream(&OutputStream);

  if (!s_pCooking->cookTriangleMesh(desc, PassThroughStream))
    return EZ_FAILURE;

  return EZ_SUCCESS;
}
