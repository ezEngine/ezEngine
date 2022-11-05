#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <RendererCore/Rasterizer/RasterizerObject.h>
#include <RendererCore/Rasterizer/Thirdparty/Occluder.h>
#include <RendererCore/Rasterizer/Thirdparty/VectorMath.h>

ezRasterizerObject::ezRasterizerObject() = default;

ezRasterizerObject::~ezRasterizerObject()
{
  if (m_pOccluder)
  {
    // TODO: the occluder class currently doesn't deallocate its own data
    _aligned_free(m_pOccluder->m_vertexData);
    m_pOccluder->m_vertexData = nullptr;

    m_pOccluder = nullptr;
  }
}

void ezRasterizerObject::Clear()
{
  m_pOccluder = nullptr;
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

void ezRasterizerObject::CreateMesh(const ezGeometry& geo)
{
  std::vector<__m128> vertices;
  vertices.reserve(geo.GetPolygons().GetCount() * 4);

  Aabb bounds;

  auto addVtx = [&](ezVec3 vtxPos)
  {
    ezSimdVec4f v;
    v.Load<4>(vtxPos.GetAsPositionVec4().GetData());
    vertices.push_back(v.m_v);
  };

  for (const auto& poly : geo.GetPolygons())
  {
    const ezUInt32 uiNumVertices = poly.m_Vertices.GetCount();
    ezUInt32 uiQuadVtx = 0;

    for (ezUInt32 i = 0; i < uiNumVertices; ++i)
    {
      if (uiQuadVtx == 4)
      {
        // TODO: restart next quad (also flip this one's front face)
        break;
      }

      const ezUInt32 vtxIdx = poly.m_Vertices[i];

      addVtx(geo.GetVertices()[vtxIdx].m_vPosition);

      bounds.include(vertices.back());
      ++uiQuadVtx;
    }

    // if the polygon is a triangle, duplicate the last vertex to make it a degenerate quad
    if (uiQuadVtx == 3)
    {
      vertices.push_back(vertices.back());
      ++uiQuadVtx;
    }

    if (uiQuadVtx == 4)
    {
      const size_t n = vertices.size();

      // swap two vertices in the quad to flip the front face (different convention between EZ and the rasterizer)
      ezMath::Swap(vertices[n - 1], vertices[n - 3]);
    }

    EZ_ASSERT_DEV(uiQuadVtx == 4, "Degenerate polygon encountered");
  }

  // pad vertices to 32 for proper alignment during baking
  while (vertices.size() % 32 != 0)
  {
    vertices.push_back(vertices[0]);
  }

  // TODO: Occluder::bake only takes std::vector, would like to use ezHybridArray instead (e.g. could take a std::span for better interop)
  m_pOccluder = Occluder::bake(vertices, bounds.m_min, bounds.m_max);
}
