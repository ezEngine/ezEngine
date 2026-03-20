#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>

#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
#  include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#  include <RendererCore/Rasterizer/Thirdparty/VectorMath.h>
#endif

ezMutex ezRasterizerObject::s_Mutex;
ezMap<ezString, ezSharedPtr<ezRasterizerObject>> ezRasterizerObject::s_Objects;

ezRasterizerObject::ezRasterizerObject() = default;
ezRasterizerObject::~ezRasterizerObject() = default;

void ezRasterizerObject::CreateMesh(const ezGeometry& geo)
{
  ezHybridArray<ezSimdVec4f, 64> vertices;
  vertices.Reserve(geo.GetPolygons().GetCount() * 4);

  ezSimdVec4f vBoundsMin(ezMath::MaxValue<float>());
  ezSimdVec4f vBoundsMax(-ezMath::MaxValue<float>());

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
      const ezVec3& pos = geo.GetVertices()[vtxIdx].m_vPosition;

      ezSimdVec4f v;
      v.Load<4>(pos.GetAsPositionVec4().GetData());
      vertices.PushBack(v);

      vBoundsMin = vBoundsMin.CompMin(v);
      vBoundsMax = vBoundsMax.CompMax(v);
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

  // Pad to 16 for alignment during generic baking
  while (vertices.GetCount() % 16 != 0)
  {
    vertices.PushBack(vertices[0]);
  }

  m_OccluderGeneric.Bake(vertices.GetData(), vertices.GetCount(), vBoundsMin, vBoundsMax);

#if EZ_ENABLED(EZ_RASTERIZER_SUPPORTED)
  // Extend padding to 32 for AVX2 baking
  while (vertices.GetCount() % 32 != 0)
  {
    vertices.PushBack(vertices[0]);
  }

  m_Occluder.bake(reinterpret_cast<const __m128*>(vertices.GetData()), vertices.GetCount(), vBoundsMin.m_v, vBoundsMax.m_v);
#endif
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

ezSharedPtr<const ezRasterizerObject> ezRasterizerObject::CreateQuadX(const ezVec2& vYZExtents)
{
  EZ_LOCK(s_Mutex);

  ezStringBuilder sName;
  sName.SetFormat("Quad-{}-{}", vYZExtents.x, vYZExtents.y);

  ezSharedPtr<ezRasterizerObject>& pObj = s_Objects[sName];

  if (pObj == nullptr)
  {
    pObj = EZ_NEW(ezFoundation::GetAlignedAllocator(), ezRasterizerObject);

    ezGeometry::GeoOptions opt;
    opt.m_MainAxis = ezBasisAxis::PositiveX;

    ezGeometry geometry;
    geometry.AddRect(vYZExtents, 1, 1, opt);

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
