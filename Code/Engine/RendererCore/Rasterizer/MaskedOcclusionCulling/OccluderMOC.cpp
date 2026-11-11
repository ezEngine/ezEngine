#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Rasterizer/MaskedOcclusionCulling/OccluderMOC.h>

void OccluderMOC::Bake(const ezDynamicArray<ezVec3>& vertices, const ezDynamicArray<ezUInt32>& triangleIndices)
{
  m_uiNumTriangles = triangleIndices.GetCount() / 3;

  // Store vertices as (x, y, z, w) with w = 1.0
  m_Vertices.SetCountUninitialized(vertices.GetCount() * 4);

  m_vBoundsMin = ezVec3(ezMath::MaxValue<float>());
  m_vBoundsMax = ezVec3(-ezMath::MaxValue<float>());

  for (ezUInt32 i = 0; i < vertices.GetCount(); ++i)
  {
    const ezVec3& v = vertices[i];
    m_Vertices[i * 4 + 0] = v.x;
    m_Vertices[i * 4 + 1] = v.y;
    m_Vertices[i * 4 + 2] = v.z;
    m_Vertices[i * 4 + 3] = 1.0f;

    m_vBoundsMin = m_vBoundsMin.CompMin(v);
    m_vBoundsMax = m_vBoundsMax.CompMax(v);
  }

  m_Indices = triangleIndices;
}
