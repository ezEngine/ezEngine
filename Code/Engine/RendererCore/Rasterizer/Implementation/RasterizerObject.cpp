#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/Rasterizer/Thirdparty/VectorMath.h>

ezMutex ezRasterizerObject::s_Mutex;
ezMap<ezString, ezSharedPtr<ezRasterizerObject>> ezRasterizerObject::s_Objects;

ezRasterizerObject::ezRasterizerObject() = default;
ezRasterizerObject::~ezRasterizerObject() = default;

#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)

// needed for ezHybridArray below
EZ_DEFINE_AS_POD_TYPE(__m128);

void ezRasterizerObject::CreateMesh(const ezGeometry& geo)
{
  ezHybridArray<__m128, 64, ezAlignedAllocatorWrapper> vertices;
  vertices.Reserve(geo.GetPolygons().GetCount() * 4);

  Aabb bounds;

  auto addVtx = [&](ezVec3 vtxPos)
  {
    ezSimdVec4f v;
    v.Load<4>(vtxPos.GetAsPositionVec4().GetData());
    vertices.PushBack(v.m_v);
  };

  for (const auto& poly : geo.GetPolygons())
  {
    const ezUInt32 uiNumVertices = poly.m_Vertices.GetCount();
    ezUInt32 uiQuadVtx = 0;

    // ignore complex polygons entirely
    if (uiNumVertices > 4)
      continue;

    for (ezUInt32 i = 0; i < uiNumVertices; ++i)
    {
      if (uiQuadVtx == 4)
      {
        // TODO: restart next quad (also flip this one's front face)
        break;
      }

      const ezUInt32 vtxIdx = poly.m_Vertices[i];

      addVtx(geo.GetVertices()[vtxIdx].m_vPosition);

      bounds.include(vertices.PeekBack());
      ++uiQuadVtx;
    }

    // if the polygon is a triangle, duplicate the last vertex to make it a degenerate quad
    if (uiQuadVtx == 3)
    {
      vertices.PushBack(vertices.PeekBack());
      ++uiQuadVtx;
    }

    if (uiQuadVtx == 4)
    {
      const ezUInt32 n = vertices.GetCount();

      // swap two vertices in the quad to flip the front face (different convention between EZ and the rasterizer)
      ezMath::Swap(vertices[n - 1], vertices[n - 3]);
    }

    EZ_ASSERT_DEV(uiQuadVtx == 4, "Degenerate polygon encountered");
  }

  // pad vertices to 32 for proper alignment during baking
  while (vertices.GetCount() % 32 != 0)
  {
    vertices.PushBack(vertices[0]);
  }

  m_Occluder.bake(vertices.GetData(), vertices.GetCount(), bounds.m_min, bounds.m_max);
}

ezSharedPtr<const ezRasterizerObject> ezRasterizerObject::GetObject(ezStringView sUniqueName)
{
  EZ_LOCK(s_Mutex);

  auto it = s_Objects.Find(sUniqueName);

  if (it.IsValid())
    return it.Value();

  return nullptr;
}

ezSharedPtr<const ezRasterizerObject> ezRasterizerObject::CreateBox(const ezVec3& vFullExtents)
{
  EZ_LOCK(s_Mutex);

  ezStringBuilder sName;
  sName.SetFormat("Box-{}-{}-{}", vFullExtents.x, vFullExtents.y, vFullExtents.z);

  ezSharedPtr<ezRasterizerObject>& pObj = s_Objects[sName];

  if (pObj == nullptr)
  {
    pObj = EZ_NEW(ezFoundation::GetAlignedAllocator(), ezRasterizerObject);

    ezGeometry geometry;
    geometry.AddBox(vFullExtents, false, {});

    pObj->CreateMesh(geometry);
  }

  return pObj;
}

ezSharedPtr<const ezRasterizerObject> ezRasterizerObject::CreateMesh(ezStringView sUniqueName, const ezGeometry& geometry)
{
  EZ_LOCK(s_Mutex);

  ezSharedPtr<ezRasterizerObject>& pObj = s_Objects[sUniqueName];

  if (pObj == nullptr)
  {
    pObj = EZ_NEW(ezFoundation::GetAlignedAllocator(), ezRasterizerObject);

    pObj->CreateMesh(geometry);
  }

  return pObj;
}

#else

void ezRasterizerObject::CreateMesh(const ezGeometry& geo)
{
}

ezSharedPtr<const ezRasterizerObject> ezRasterizerObject::GetObject(ezStringView sUniqueName)
{
  return nullptr;
}

ezSharedPtr<const ezRasterizerObject> ezRasterizerObject::CreateBox(const ezVec3& vFullExtents)
{
  return nullptr;
}

ezSharedPtr<const ezRasterizerObject> ezRasterizerObject::CreateMesh(ezStringView sUniqueName, const ezGeometry& geometry)
{
  return nullptr;
}

#endif


