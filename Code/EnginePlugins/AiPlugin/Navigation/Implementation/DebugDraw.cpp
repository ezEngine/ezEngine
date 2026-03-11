#include <AiPlugin/AiPluginPCH.h>

#include <DetourNavMesh.h>
#include <RendererCore/Debug/DebugRenderer.h>

static float DistancePoint2Line2D(const float* pPoint, const float* pLine1, const float* pLine2)
{
  float pqx = pLine2[0] - pLine1[0];
  float pqz = pLine2[2] - pLine1[2];
  float dx = pPoint[0] - pLine1[0];
  float dz = pPoint[2] - pLine1[2];
  float d = pqx * pqx + pqz * pqz;
  float t = pqx * dx + pqz * dz;
  if (d != 0)
    t /= d;
  dx = pLine1[0] + t * pqx - pPoint[0];
  dz = pLine1[2] + t * pqz - pPoint[2];
  return dx * dx + dz * dz;
}

void DrawMeshTilePolygons(const dtMeshTile& meshTile, ezDynamicArray<ezDebugRendererTriangle>& out_triangles, ezArrayPtr<ezColor> areaColors)
{
  for (int polyIdx = 0; polyIdx < meshTile.header->polyCount; ++polyIdx)
  {
    const dtPoly& poly = meshTile.polys[polyIdx];

    if (poly.getType() == DT_POLYTYPE_OFFMESH_CONNECTION) // Skip off-mesh links
      continue;

    const dtPolyDetail& polyDetail = meshTile.detailMeshes[polyIdx];

    for (int triIdx = 0; triIdx < polyDetail.triCount; ++triIdx)
    {
      const ezUInt8* pTriangle = &meshTile.detailTris[(polyDetail.triBase + triIdx) * 4]; // 3 indices, one byte flags

      ezDebugRendererTriangle& tri = out_triangles.ExpandAndGetRef();

      ezUInt8 uiArea = poly.getArea();
      tri.m_color = areaColors[uiArea];

      for (int vtxIdx = 0; vtxIdx < 3; ++vtxIdx)
      {
        const ezUInt8 vtxValue = pTriangle[vtxIdx];
        const float* vtxPos = nullptr;

        if (vtxValue < poly.vertCount)
        {
          vtxPos = &meshTile.verts[poly.verts[vtxValue] * 3];
        }
        else
        {
          vtxPos = &meshTile.detailVerts[(polyDetail.vertBase + vtxValue - poly.vertCount) * 3];
        }

        tri.m_position[vtxIdx].Set(vtxPos[0], vtxPos[2], vtxPos[1] + 0.05f); // swap Y and Z and move it up slightly
      }

      ezMath::Swap(tri.m_position[1], tri.m_position[2]);                    // fix the triangle winding
    }
  }
}

void DrawMeshTileEdges(const dtMeshTile& meshTile, bool bOuterEdges, bool bInnerEdges, bool bInnerDetailEdges, ezDynamicArray<ezDebugRendererLine>& out_lines)
{
  constexpr float fThreshold = 0.01f * 0.01f;

  for (int polyIdx = 0; polyIdx < meshTile.header->polyCount; ++polyIdx)
  {
    const dtPoly* p = &meshTile.polys[polyIdx];

    if (p->getType() == DT_POLYTYPE_OFFMESH_CONNECTION)
      continue; // Skip off-mesh links.

    const dtPolyDetail& detailMesh = meshTile.detailMeshes[polyIdx];

    const int numPolyVertices = (int)p->vertCount;

    for (int vtxIdx = 0; vtxIdx < numPolyVertices; ++vtxIdx)
    {
      const bool bIsInnerEdge = p->neis[vtxIdx] != 0;

      if (bIsInnerEdge && !bInnerEdges)
        continue; // Skip inner edges.

      if (!bIsInnerEdge && !bOuterEdges)
        continue; // Skip outer edges.

      ezColor edgeColor = ezColor::White;

      if (bIsInnerEdge)
      {
        if (p->neis[vtxIdx] & DT_EXT_LINK) // Border between tiles.
        {
          bool bIsConnected = false;       // Connected to solid edge.

          for (ezUInt32 linkIdx = p->firstLink; linkIdx != DT_NULL_LINK; linkIdx = meshTile.links[linkIdx].next)
          {
            if (meshTile.links[linkIdx].edge == vtxIdx)
            {
              bIsConnected = true;
              break;
            }
          }

          if (bIsConnected)
            edgeColor = ezColor::Blue; // border with a neighboring tile that is loaded
          else
            edgeColor = ezColor::Red;  // border with a neighboring tile that is not loaded
        }
        else
        {
          if (!bInnerDetailEdges)
            continue;                    // Skip inner detail edges.

          edgeColor = ezColor::DarkGrey; // inner detail region boundary
        }
      }
      else
      {
        edgeColor = ezColor::Yellow; // outer boundary color
      }

      const float* v0 = &meshTile.verts[p->verts[vtxIdx] * 3];
      const float* v1 = &meshTile.verts[p->verts[(vtxIdx + 1) % numPolyVertices] * 3];

      // Draw detail mesh edges which align with the actual poly edge.
      for (int k = 0; k < detailMesh.triCount; ++k)
      {
        const ezUInt8* t = &meshTile.detailTris[(detailMesh.triBase + k) * 4];
        const float* tv[3];

        for (ezInt32 m = 0; m < 3; ++m)
        {
          if (t[m] < p->vertCount)
            tv[m] = &meshTile.verts[p->verts[t[m]] * 3];
          else
            tv[m] = &meshTile.detailVerts[(detailMesh.vertBase + (t[m] - p->vertCount)) * 3];
        }

        for (ezInt32 m = 0, n = 2; m < 3; n = m++)
        {
          if ((dtGetDetailTriEdgeFlags(t[3], n) & DT_DETAIL_EDGE_BOUNDARY) == 0)
            continue;

          if (DistancePoint2Line2D(tv[n], v0, v1) < fThreshold && DistancePoint2Line2D(tv[m], v0, v1) < fThreshold)
          {
            auto& line = out_lines.ExpandAndGetRef();
            line.m_startColor = edgeColor;
            line.m_endColor = edgeColor;
            line.m_start.Set(tv[n][0], tv[n][2], tv[n][1] + 0.05f);
            line.m_end.Set(tv[m][0], tv[m][2], tv[m][1] + 0.05f);
          }
        }
      }
    }
  }
}
