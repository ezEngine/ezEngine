#include <PCH.h>
#include <PhysXCooking/PhysXCooking.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Configuration/Singleton.h>

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

  PxTriangleMeshDesc desc;
  desc.setToDefault();
  desc.materialIndices.data = mesh.m_PolygonSurfaceID.GetData();
  desc.materialIndices.stride = sizeof(ezUInt16);

  ezDynamicArray<ezUInt32> TriangleIndices;
  CreateMeshDesc(mesh, desc, TriangleIndices);

  ezPxOutStream PassThroughStream(&OutputStream);

  if (!s_pCooking->cookTriangleMesh(desc, PassThroughStream))
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

ezResult ezPhysXCooking::CookConvexMesh(const ezPhysXCookingMesh& mesh0, ezStreamWriter& OutputStream)
{
  ezPhysXCookingMesh mesh;
  if (ComputeConvexHull(mesh0, mesh).Failed())
  {
    ezLog::Error("Convex Hull computation failed.");
    return EZ_FAILURE;
  }

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

  ezDynamicArray<ezUInt32> TriangleIndices;
  CreateMeshDesc(mesh, desc, TriangleIndices);

  if (desc.triangles.count > 255)
  {
    ezLog::Error("Cannot cook convex meshes with more than 255 triangles. This mesh has {0}.", desc.triangles.count);
    return EZ_FAILURE;
  }

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

class ezConvexHullGenerator
{
public:
  struct Face
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt16 m_uiVertexIdx[3];
  };

  ezResult Build(const ezArrayPtr<const ezVec3> vertices);
  void Retrieve(ezDynamicArray<ezVec3>& out_Vertices, ezDynamicArray<Face>& out_Faces);

private:
  ezResult ComputeCenterAndScale(const ezArrayPtr<const ezVec3> vertices);
  ezResult StoreNormalizedVertices(const ezArrayPtr<const ezVec3> vertices);
  void StoreTriangle(int i, int j, int k);
  void InitializeHull();
  void ComputeHull();
  void RemoveVisibleFaces(ezUInt32 vtxId);
  void PatchHole(ezUInt32 vtxId);

  struct TwoSet
  {
    EZ_ALWAYS_INLINE TwoSet() { a = 0xFFFF; b = 0xFFFF; }
    EZ_ALWAYS_INLINE void Add(ezUInt16 x) { (a == 0xFFFF ? a : b) = x; }
    EZ_ALWAYS_INLINE bool Contains(ezUInt16 x) { return a == x || b == x; }
    EZ_ALWAYS_INLINE void Remove(ezUInt16 x) { (a == x ? a : b) = 0xFFFF; }
    EZ_ALWAYS_INLINE int GetSize() { return (a != 0xFFFF) + (b != 0xFFFF); }

    ezUInt16 a, b;
  };

  struct Triangle
  {
    ezVec3d m_vNormal;
    double m_fPlaneDistance;
    ezUInt16 m_uiVertexIdx[3];
    bool m_bFlip;
  };

  ezVec3d m_vCenter;
  double m_fScale;

  // all the 'good' vertices (no duplicates)
  // normalized to be within a unit-cube
  ezDynamicArray<ezVec3d> m_Vertices;

  // Will be resized to Square(m_Vertices.GetCount())
  // Index [i * m_Vertices.GetCount() + j] indicates which (up to two) other points
  // combine with the edge i and j to make a triangle in the hull.  Only defined when i < j.
  ezDynamicArray<TwoSet> m_Edges;

  ezDeque<Triangle> m_Triangles;
};

ezResult ezConvexHullGenerator::ComputeCenterAndScale(const ezArrayPtr<const ezVec3> vertices)
{
  if (vertices.IsEmpty())
    return EZ_FAILURE;

  ezBoundingBox box;
  box.SetFromPoints(vertices.GetPtr(), vertices.GetCount());

  const ezVec3 c = box.GetCenter();
  m_vCenter.Set(c.x, c.y, c.z);

  const ezVec3 ext = box.GetHalfExtents();

  const double minExt = ezMath::Min(ext.x, ext.y, ext.z);

  if (minExt <= 0.000001)
    return EZ_FAILURE;

  const double maxExt = ezMath::Max(ext.x, ext.y, ext.z);

  m_fScale = 1.0 / maxExt;
  //m_vCenter.SetZero();
  //m_fScale = 1.0;
  return EZ_SUCCESS;
}

ezResult ezConvexHullGenerator::StoreNormalizedVertices(const ezArrayPtr<const ezVec3> vertices)
{
  struct Comparer
  {
    EZ_ALWAYS_INLINE bool Less(const ezVec3d& a, const ezVec3d& b) const
    {
      constexpr double eps = 0.01;

      if (a.x < b.x - eps)
        return true;
      if (a.x > b.x + eps)
        return false;

      if (a.y < b.y - eps)
        return true;
      if (a.y > b.y + eps)
        return false;

      return (a.z < b.z - eps);
    }
  };

  ezSet<ezVec3d, Comparer> used;

  m_Vertices.Clear();
  m_Vertices.Reserve(vertices.GetCount());

  for (ezVec3 v : vertices)
  {
    ezVec3d norm;
    norm.Set(v.x, v.y, v.z);

    // bring into [-1; +1] range for normalized precision
    norm -= m_vCenter;
    norm *= m_fScale;

    if (!used.Contains(norm))
    {
      m_Vertices.PushBack(norm);
      used.Insert(norm);
    }
  }

  if (m_Vertices.GetCount() < 4)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

void ezConvexHullGenerator::StoreTriangle(int i, int j, int k)
{
  Triangle& triangle = m_Triangles.ExpandAndGetRef();

  EZ_ASSERT_DEBUG(i < j, "Invalid Triangle");
  EZ_ASSERT_DEBUG(i < k, "Invalid Triangle");
  EZ_ASSERT_DEBUG(j < k, "Invalid Triangle");

  triangle.m_vNormal = (m_Vertices[k] - m_Vertices[i]).Cross(m_Vertices[j] - m_Vertices[i]);
  triangle.m_vNormal.Normalize();
  triangle.m_fPlaneDistance = triangle.m_vNormal.Dot(m_Vertices[i]);

  triangle.m_uiVertexIdx[0] = i;
  triangle.m_uiVertexIdx[1] = j;
  triangle.m_uiVertexIdx[2] = k;

  const ezUInt32 uiMaxVertices = m_Vertices.GetCount();

  m_Edges[i * uiMaxVertices + j].Add(k);
  m_Edges[i * uiMaxVertices + k].Add(j);
  m_Edges[j * uiMaxVertices + k].Add(i);

  ezVec3d avg;
  {
    avg.SetZero();
    for (ezUInt32 v = 0; v < 4; ++v)
      avg += m_Vertices[v];
    avg /= 4.0;
  }

  //triangle.m_bFlip = triangle.m_fPlaneDistance < 0;
  triangle.m_bFlip = triangle.m_vNormal.Dot(avg) > triangle.m_fPlaneDistance;

  if (triangle.m_bFlip)
  {
    triangle.m_vNormal = -triangle.m_vNormal;
    triangle.m_fPlaneDistance = -triangle.m_fPlaneDistance;
  }
}

void ezConvexHullGenerator::InitializeHull()
{
  // Initially construct the hull as containing only the first four points.
  for (int i = 0; i < 4; i++)
  {
    for (int j = i + 1; j < 4; j++)
    {
      for (int k = j + 1; k < 4; k++)
      {
        StoreTriangle(i, j, k);
      }
    }
  }
}

void ezConvexHullGenerator::ComputeHull()
{
  const ezUInt32 uiMaxVertices = m_Vertices.GetCount();

  // Add the points to the hull, one at a time.
  for (ezUInt32 vtxId = 4; vtxId < uiMaxVertices; ++vtxId)
  {
    // Find and delete all faces with their outside 'illuminated' by this point.
    RemoveVisibleFaces(vtxId);

    // Now for any edge still in the hull that is only part of one face
    // add another face containing the new point and that edge to the hull.
    PatchHole(vtxId);
  }
}

void ezConvexHullGenerator::RemoveVisibleFaces(ezUInt32 vtxId)
{
  const ezUInt32 uiMaxVertices = m_Vertices.GetCount();
  const ezVec3d pos = m_Vertices[vtxId];

  ezInt32 iNumTriangles = m_Triangles.GetCount();
  for (ezInt32 j = 0; j < iNumTriangles; j++)
  {
    const auto& tri = m_Triangles[j];

    const double dist = tri.m_vNormal.Dot(pos);
    if (dist <= tri.m_fPlaneDistance)
      continue;

    const ezUInt16 vtx0 = tri.m_uiVertexIdx[0];
    const ezUInt16 vtx1 = tri.m_uiVertexIdx[1];
    const ezUInt16 vtx2 = tri.m_uiVertexIdx[2];

    m_Edges[vtx0 * uiMaxVertices + vtx1].Remove(vtx2);
    m_Edges[vtx0 * uiMaxVertices + vtx2].Remove(vtx1);
    m_Edges[vtx1 * uiMaxVertices + vtx2].Remove(vtx0);

    m_Triangles.RemoveAtSwap(j);

    --j;
    --iNumTriangles;
  }
}

void ezConvexHullGenerator::PatchHole(ezUInt32 vtxId)
{
  const ezUInt32 uiMaxVertices = m_Vertices.GetCount();

  const ezUInt32 uiNumFaces = m_Triangles.GetCount();
  for (ezUInt32 j = 0; j < uiNumFaces; j++)
  {
    const auto& tri = m_Triangles[j];

    for (ezInt32 a = 0; a < 3; a++)
    {
      for (ezInt32 b = a + 1; b < 3; b++)
      {
        const ezUInt16 vtxA = tri.m_uiVertexIdx[a];
        const ezUInt16 vtxB = tri.m_uiVertexIdx[b];

        if (m_Edges[vtxA * uiMaxVertices + vtxB].GetSize() == 2)
          continue;

        StoreTriangle(vtxA, vtxB, vtxId);
      }
    }
  }
}

ezResult ezConvexHullGenerator::Build(const ezArrayPtr<const ezVec3> vertices)
{
  m_Vertices.Clear();
  m_Edges.Clear();
  m_Triangles.Clear();

  EZ_SUCCEED_OR_RETURN(ComputeCenterAndScale(vertices));
  EZ_SUCCEED_OR_RETURN(StoreNormalizedVertices(vertices));

  if (m_Vertices.GetCount() >= 0xFFF0)
    return EZ_FAILURE;

  m_Edges.SetCount(ezMath::Square(m_Vertices.GetCount()));
  m_Triangles.Reserve(512);

  InitializeHull();
  ComputeHull();

  if (m_Triangles.GetCount() < 4)
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

void ezConvexHullGenerator::Retrieve(ezDynamicArray<ezVec3>& out_Vertices, ezDynamicArray<Face>& out_Faces)
{
  out_Vertices.Clear();
  out_Faces.Clear();

  out_Vertices.Reserve(m_Triangles.GetCount() * 2);
  out_Faces.Reserve(m_Triangles.GetCount());

  ezMap<ezUInt32, ezUInt32> vtxMap;

  const double fScaleBack = 1.0 / m_fScale;

  for (const auto& tri : m_Triangles)
  {
    auto& face = out_Faces.ExpandAndGetRef();

    for (int v = 0; v < 3; ++v)
    {
      const ezUInt32 orgIdx = tri.m_uiVertexIdx[v];

      bool bExisted = false;
      auto it = vtxMap.FindOrAdd(orgIdx, &bExisted);
      if (!bExisted)
      {
        it.Value() = out_Vertices.GetCount();

        const ezVec3d pos = (m_Vertices[orgIdx] * fScaleBack) + m_vCenter;

        ezVec3& vtx = out_Vertices.ExpandAndGetRef();
        vtx.Set((float)pos.x, (float)pos.y, (float)pos.z);
      }

      face.m_uiVertexIdx[v] = it.Value();
    }

    if (tri.m_bFlip)
    {
      ezMath::Swap(face.m_uiVertexIdx[1], face.m_uiVertexIdx[2]);
    }
  }
}

ezResult ezPhysXCooking::ComputeConvexHull(const ezPhysXCookingMesh& mesh, ezPhysXCookingMesh& outMesh)
{
  outMesh.m_bFlipNormals = mesh.m_bFlipNormals;

  ezConvexHullGenerator gen;
  EZ_SUCCEED_OR_RETURN(gen.Build(mesh.m_Vertices));

  ezDynamicArray<ezConvexHullGenerator::Face> faces;
  gen.Retrieve(outMesh.m_Vertices, faces);

  for (const auto& face : faces)
  {
    outMesh.m_VerticesInPolygon.ExpandAndGetRef() = 3;
    outMesh.m_PolygonSurfaceID.ExpandAndGetRef() = 0;

    for (int vert = 0; vert < 3; ++vert)
      outMesh.m_PolygonIndices.ExpandAndGetRef() = face.m_uiVertexIdx[vert];
  }

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

