#include <PCH.h>
#include <PhysXCooking/PhysXCooking.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Configuration/Singleton.h>
#include <Core/Graphics/ConvexHull.h>
#include <PxPhysicsAPI.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/Progress.h>

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
  s_pPhysX = ezSingletonRegistry::GetSingletonInstance<ezPhysXInterface>("ezPhysXInterface");

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

class ezPxAllocator : public PxAllocatorCallback
{
public:

  virtual void* allocate(size_t size, const char* typeName, const char* filename, int line) override
  {
    if (size == 0)
      return nullptr;

    return new unsigned char[size];
  }

  virtual void deallocate(void* ptr) override
  {
    if (ptr != nullptr)
    {
      delete[] ptr;
    }
  }

};

ezResult ezPhysXCooking::CookTriangleMesh(const ezPhysXCookingMesh& mesh, ezStreamWriter& OutputStream)
{
  //ezPhysXCookingMesh mesh;
  //if (ComputeConvexHull(mesh0, mesh).Failed())
  //{
  //  ezLog::Error("Convex Hull computation failed.");
  //  return EZ_FAILURE;
  //}

  ezProgressRange range("Cooking Triangle Mesh", 2, false);

  PxTriangleMeshDesc desc;
  desc.setToDefault();
  desc.materialIndices.data = mesh.m_PolygonSurfaceID.GetData();
  desc.materialIndices.stride = sizeof(ezUInt16);

  range.BeginNextStep("Preprocessing Mesh");

  ezDynamicArray<ezUInt32> TriangleIndices;
  CreateMeshDesc(mesh, desc, TriangleIndices);

  ezPxOutStream PassThroughStream(&OutputStream);

  range.BeginNextStep("PhysX Cooking");

  if (!s_pCooking->cookTriangleMesh(desc, PassThroughStream))
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezPhysXCooking::CookConvexMesh(const ezPhysXCookingMesh& mesh0, ezStreamWriter& OutputStream)
{
  ezProgressRange range("Cooking Convex Mesh", 4, false);

  range.BeginNextStep("Computing Convex Hull");

  ezPhysXCookingMesh mesh;
  EZ_SUCCEED_OR_RETURN(ComputeConvexHull(mesh0, mesh));

  if (mesh.m_PolygonIndices.IsEmpty() || mesh.m_Vertices.IsEmpty())
  {
    ezLog::Error("Convex mesh data is empty.");
    return EZ_FAILURE;
  }

  if (mesh.m_VerticesInPolygon.GetCount() > 255)
  {
    ezLog::Error("Cannot cook convex meshes with more than 255 polygons. This mesh has {0}.", mesh.m_VerticesInPolygon.GetCount());
    return EZ_FAILURE;
  }

  PxSimpleTriangleMesh desc;
  desc.setToDefault();

  range.BeginNextStep("Preparing Mesh Data");

  ezDynamicArray<ezUInt32> TriangleIndices;
  CreateMeshDesc(mesh, desc, TriangleIndices);

  if (desc.triangles.count > 255)
  {
    ezLog::Error("Cannot cook convex meshes with more than 255 triangles. This mesh has {0}.", desc.triangles.count);
    return EZ_FAILURE;
  }

  range.BeginNextStep("Computing Hull Polygons");

  ezPxAllocator allocator;

  PxU32 uiNumVertices = desc.points.count, uiNumIndices = desc.triangles.count * 3, uiNumPolygons = 0;
  PxVec3* pVertices = nullptr;
  PxU32* pIndices = nullptr;
  PxHullPolygon* pPolygons = nullptr;
  if (!s_pCooking->computeHullPolygons(desc, allocator, uiNumVertices, pVertices, uiNumIndices, pIndices, uiNumPolygons, pPolygons))
  {
    ezLog::Error("Convex Hull computation failed");
    allocator.deallocate(pVertices);
    allocator.deallocate(pIndices);
    allocator.deallocate(pPolygons);
    return EZ_FAILURE;
  }

  PxConvexMeshDesc convex;
  convex.points.count = uiNumVertices;
  convex.points.data = pVertices;
  convex.points.stride = sizeof(PxVec3);

  convex.indices.count = uiNumIndices;
  convex.indices.data = pIndices;
  convex.indices.stride = sizeof(PxU32);

  convex.polygons.count = uiNumPolygons;
  convex.polygons.data = pPolygons;
  convex.polygons.stride = sizeof(PxHullPolygon);

  convex.vertexLimit = 256;

  range.BeginNextStep("PhysX Cooking");

  ezPxOutStream PassThroughStream(&OutputStream);
  if (!s_pCooking->cookConvexMesh(convex, PassThroughStream))
  {
    ezLog::Warning("Convex mesh cooking failed. Trying again with inflated mesh.");

    convex.flags.set(PxConvexFlag::eCOMPUTE_CONVEX);
    convex.flags.set(PxConvexFlag::eINFLATE_CONVEX);

    if (!s_pCooking->cookConvexMesh(convex, PassThroughStream))
    {
      allocator.deallocate(pVertices);
      allocator.deallocate(pIndices);
      allocator.deallocate(pPolygons);

      ezLog::Error("Convex mesh cooking failed with inflated mesh as well.");
      return EZ_FAILURE;
    }
  }

  allocator.deallocate(pVertices);
  allocator.deallocate(pIndices);
  allocator.deallocate(pPolygons);
  return EZ_SUCCESS;
}

ezResult ezPhysXCooking::ComputeConvexHull(const ezPhysXCookingMesh& mesh, ezPhysXCookingMesh& outMesh)
{
  ezStopwatch timer;

  outMesh.m_bFlipNormals = mesh.m_bFlipNormals;

  ezConvexHullGenerator gen;
  if (gen.Build(mesh.m_Vertices).Failed())
  {
    ezLog::Error("Computing the convex hull failed.");
    return EZ_FAILURE;
  }

  ezDynamicArray<ezConvexHullGenerator::Face> faces;
  gen.Retrieve(outMesh.m_Vertices, faces);

  for (const auto& face : faces)
  {
    outMesh.m_VerticesInPolygon.ExpandAndGetRef() = 3;
    outMesh.m_PolygonSurfaceID.ExpandAndGetRef() = 0;

    for (int vert = 0; vert < 3; ++vert)
      outMesh.m_PolygonIndices.ExpandAndGetRef() = face.m_uiVertexIdx[vert];
  }

  ezLog::Dev("Computed the convex hull in {0} milliseconds", ezArgF(timer.GetRunningTotal().GetMilliseconds(), 1));
  return EZ_SUCCESS;
}

void ezPhysXCooking::CreateMeshDesc(const ezPhysXCookingMesh& mesh, PxSimpleTriangleMesh& desc, ezDynamicArray<ezUInt32>& TriangleIndices)
{
  desc.setToDefault();

  desc.points.count = mesh.m_Vertices.GetCount();
  desc.points.stride = sizeof(ezVec3);
  desc.points.data = mesh.m_Vertices.GetData();

  ezUInt32 uiTriangles = 0;
  for (auto numIndices : mesh.m_VerticesInPolygon)
    uiTriangles += numIndices - 2;

  TriangleIndices.SetCountUninitialized(uiTriangles * 3);

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
}



EZ_STATICLINK_FILE(PhysXCooking, PhysXCooking_PhysXCooking);

