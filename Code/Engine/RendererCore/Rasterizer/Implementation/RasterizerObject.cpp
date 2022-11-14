#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/Rasterizer/Thirdparty/VectorMath.h>

ezRasterizerObject::ezRasterizerObject() = default;

ezRasterizerObject::~ezRasterizerObject()
{
  Clear();
}

void ezRasterizerObject::Clear()
{
  m_Occluder.Clear();
}

void ezRasterizerObject::CreateBox(const ezVec3& vFullExtents, const ezMat4& mTransform)
{
  ezGeometry::GeoOptions opt;
  ezGeometry geo;

  opt.m_Transform = mTransform;
  geo.AddBox(vFullExtents, false, opt);

  CreateMesh(geo);
}

void ezRasterizerObject::CreateSphere(float fRadius, const ezMat4& mTransform)
{
  ezGeometry::GeoOptions opt;
  ezGeometry geo;

  opt.m_Transform = mTransform;
  geo.AddGeodesicSphere(fRadius, 0, opt);

  CreateMesh(geo);
}

// needed for ezHybridArray below
EZ_DEFINE_AS_POD_TYPE(__m128);

void ezRasterizerObject::CreateMesh(const ezGeometry& geo)
{
  ezHybridArray<__m128, 64, ezAlignedAllocatorWrapper> vertices;
  vertices.Reserve(geo.GetPolygons().GetCount() * 4);

  Aabb bounds;

  auto addVtx = [&](ezVec3 vtxPos) {
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
      const size_t n = vertices.GetCount();

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
