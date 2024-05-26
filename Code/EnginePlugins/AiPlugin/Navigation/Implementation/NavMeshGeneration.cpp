#include <AiPlugin/Navigation/Implementation/NavMeshGeneration.h>
#include <AiPlugin/Navigation/NavMesh.h>
#include <Core/Interfaces/NavmeshGeoWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <Recast.h>
#include <cstdint>

void FillOutConfig(rcConfig& ref_cfg, const ezAiNavmeshConfig& config, const ezBoundingBox& bbox)
{
  ezMemoryUtils::ZeroFill(&ref_cfg, 1);
  ref_cfg.bmin[0] = bbox.m_vMin.x;
  ref_cfg.bmin[1] = bbox.m_vMin.z;
  ref_cfg.bmin[2] = bbox.m_vMin.y;
  ref_cfg.bmax[0] = bbox.m_vMax.x;
  ref_cfg.bmax[1] = bbox.m_vMax.z;
  ref_cfg.bmax[2] = bbox.m_vMax.y;
  ref_cfg.ch = config.m_fCellHeight;
  ref_cfg.cs = config.m_fCellSize;
  ref_cfg.walkableSlopeAngle = config.m_WalkableSlope.GetDegree();
  ref_cfg.walkableHeight = (int)ceilf(config.m_fAgentHeight / ref_cfg.ch);
  ref_cfg.walkableClimb = (int)floorf(config.m_fAgentStepHeight / ref_cfg.ch);
  ref_cfg.walkableRadius = (int)ceilf(config.m_fAgentRadius / ref_cfg.cs);
  ref_cfg.maxEdgeLen = (int)(config.m_fMaxEdgeLength / ref_cfg.cs);
  ref_cfg.maxSimplificationError = config.m_fMaxSimplificationError;
  ref_cfg.minRegionArea = (int)ezMath::Square(config.m_fMinRegionSize);
  ref_cfg.mergeRegionArea = (int)ezMath::Square(config.m_fRegionMergeSize);
  ref_cfg.maxVertsPerPoly = 6;
  ref_cfg.detailSampleDist = config.m_fDetailMeshSampleDistanceFactor < 0.9f ? 0 : ref_cfg.cs * config.m_fDetailMeshSampleDistanceFactor;
  ref_cfg.detailSampleMaxError = ref_cfg.ch * config.m_fDetailMeshSampleErrorFactor;
  ref_cfg.borderSize = ref_cfg.walkableRadius + 3; // Reserve enough padding.

  ref_cfg.bmin[0] -= ref_cfg.borderSize * ref_cfg.cs;
  ref_cfg.bmin[2] -= ref_cfg.borderSize * ref_cfg.cs;
  ref_cfg.bmax[0] += ref_cfg.borderSize * ref_cfg.cs;
  ref_cfg.bmax[2] += ref_cfg.borderSize * ref_cfg.cs;

  rcCalcGridSize(ref_cfg.bmin, ref_cfg.bmax, ref_cfg.cs, &ref_cfg.width, &ref_cfg.height);
}

ezResult BuildRecastPolyMesh(const ezAiNavmeshConfig& config, ezBoundingBox aabb, rcPolyMesh& out_polyMesh, rcContext* pContext, ezArrayPtr<const ezVec3> vertices, ezArrayPtr<const ezAiNavMeshTriangle> triangles, ezArrayPtr<ezUInt8> triangleAreaIDs)
{
  const float* pVertices = &vertices[0].x;
  const ezInt32* pTriangles = &triangles[0].m_VertexIdx[0];

  // adjust the bounding box to the data that we got (height only)
  {
    float fMinY = ezMath::HighValue<float>();
    float fMaxY = -ezMath::HighValue<float>();
    for (const ezVec3& v : vertices)
    {
      fMinY = ezMath::Min(fMinY, v.y);
      fMaxY = ezMath::Max(fMaxY, v.y);
    }

    aabb.m_vMin.z = fMinY;
    aabb.m_vMax.z = fMaxY;
  }

  rcConfig cfg;
  FillOutConfig(cfg, config, aabb);

  rcHeightfield* heightfield = rcAllocHeightfield();
  EZ_SCOPE_EXIT(rcFreeHeightField(heightfield));

  if (!rcCreateHeightfield(pContext, *heightfield, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
  {
    ezLog::Error("[AI]Could not create solid heightfield for navmesh.");
    return EZ_FAILURE;
  }

  rcClearUnwalkableTriangles(pContext, cfg.walkableSlopeAngle, pVertices, vertices.GetCount(), pTriangles, triangles.GetCount(), triangleAreaIDs.GetPtr());

  if (!rcRasterizeTriangles(pContext, pVertices, vertices.GetCount(), pTriangles, triangleAreaIDs.GetPtr(), triangles.GetCount(), *heightfield, cfg.walkableClimb))
  {
    ezLog::Error("[AI]Could not rasterize navmesh triangles.");
    return EZ_FAILURE;
  }

  rcFilterLowHangingWalkableObstacles(pContext, cfg.walkableClimb, *heightfield);

  rcFilterLedgeSpans(pContext, cfg.walkableHeight, cfg.walkableClimb, *heightfield);

  rcFilterWalkableLowHeightSpans(pContext, cfg.walkableHeight, *heightfield);

  rcCompactHeightfield* compactHeightfield = rcAllocCompactHeightfield();
  EZ_SCOPE_EXIT(rcFreeCompactHeightfield(compactHeightfield));

  if (!rcBuildCompactHeightfield(pContext, cfg.walkableHeight, cfg.walkableClimb, *heightfield, *compactHeightfield))
  {
    ezLog::Error("[AI]Could not build compact navmesh data.");
    return EZ_FAILURE;
  }

  if (!rcErodeWalkableArea(pContext, cfg.walkableRadius, *compactHeightfield))
  {
    ezLog::Error("[AI]Could not erode navmesh with character radius");
    return EZ_FAILURE;
  }

  // Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.

  // PARTITION_WATERSHED
  if (false)
  {
    // Prepare for region partitioning, by calculating distance field along the walkable surface.
    if (!rcBuildDistanceField(pContext, *compactHeightfield))
    {
      ezLog::Error("[AI]Could not build navmesh distance field.");
      return EZ_FAILURE;
    }

    // Partition the walkable surface into simple regions without holes.
    if (!rcBuildRegions(pContext, *compactHeightfield, cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea))
    {
      ezLog::Error("[AI]Could not build navmesh watershed regions.");
      return EZ_FAILURE;
    }
  }

  // PARTITION_MONOTONE
  if (false)
  {
    // Partition the walkable surface into simple regions without holes.
    // Monotone partitioning does not need distance field.
    if (!rcBuildRegionsMonotone(pContext, *compactHeightfield, cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea))
    {
      ezLog::Error("[AI]Could not build monotone navmesh regions.");
      return EZ_FAILURE;
    }
  }

  // PARTITION_LAYERS
  if (true)
  {
    // Partition the walkable surface into simple regions without holes.
    if (!rcBuildLayerRegions(pContext, *compactHeightfield, cfg.borderSize, cfg.minRegionArea))
    {
      ezLog::Error("[AI]Could not build navmesh layer regions.");
      return EZ_FAILURE;
    }
  }

  rcContourSet* contourSet = rcAllocContourSet();
  EZ_SCOPE_EXIT(rcFreeContourSet(contourSet));

  if (!rcBuildContours(pContext, *compactHeightfield, cfg.maxSimplificationError, cfg.maxEdgeLen, *contourSet))
  {
    ezLog::Error("[AI]Could not create navmesh contours");
    return EZ_FAILURE;
  }

  if (!rcBuildPolyMesh(pContext, *contourSet, cfg.maxVertsPerPoly, out_polyMesh))
  {
    ezLog::Error("[AI]Could not triangulate navmesh contours");
    return EZ_FAILURE;
  }

  //////////////////////////////////////////////////////////////////////////
  // Detour Navmesh

  for (int i = 0; i < out_polyMesh.npolys; ++i)
  {
    if (out_polyMesh.areas[i] != RC_NULL_AREA)
    {
      out_polyMesh.flags[i] = 0xFFFF;
    }
    else
    {
      out_polyMesh.flags[i] = 0;
    }
  }

  return EZ_SUCCESS;
}

ezResult BuildDetourNavMeshData(const ezAiNavmeshConfig& config, const rcPolyMesh& polyMesh, ezDataBuffer& out_navmeshData, ezVec2I32 vSectorCoord)
{
  dtNavMeshCreateParams params;
  ezMemoryUtils::ZeroFill(&params, 1);

  params.verts = polyMesh.verts;
  params.vertCount = polyMesh.nverts;
  params.polys = polyMesh.polys;
  params.polyAreas = polyMesh.areas;
  params.polyFlags = polyMesh.flags;
  params.polyCount = polyMesh.npolys;
  params.nvp = polyMesh.nvp;
  params.walkableHeight = config.m_fAgentHeight;
  params.walkableRadius = config.m_fAgentRadius;
  params.walkableClimb = config.m_fAgentStepHeight;
  rcVcopy(params.bmin, polyMesh.bmin);
  rcVcopy(params.bmax, polyMesh.bmax);
  params.cs = config.m_fCellSize;
  params.ch = config.m_fCellHeight;
  params.buildBvTree = true;
  params.tileLayer = 0;
  params.tileX = vSectorCoord.x;
  params.tileY = vSectorCoord.y;

  ezInt32 navDataSize = 0;
  ezUInt8* navData = nullptr;
  EZ_SCOPE_EXIT(dtFree(navData));

  if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
  {
    ezLog::Error("Could not build Detour navmesh.");
    return EZ_FAILURE;
  }

  out_navmeshData.SetCountUninitialized(navDataSize);
  ezMemoryUtils::Copy(out_navmeshData.GetData(), navData, navDataSize);

  return EZ_SUCCESS;
}

void ezNavMeshSectorGenerationTask::Execute()
{
  m_pWorldNavMesh->BuildSector(m_SectorID, m_pNavGeo);
}

static ezInt8 GetSurfaceGroundType(const ezSurfaceResource* pSurf)
{
  while (pSurf)
  {
    const auto& desc = pSurf->GetDescriptor();
    pSurf = nullptr;

    if (desc.m_iGroundType >= 0)
    {
      return desc.m_iGroundType;
    }
    else if (desc.m_hBaseSurface.IsValid())
    {
      ezResourceLock<ezSurfaceResource> pRes(desc.m_hBaseSurface, ezResourceAcquireMode::BlockTillLoaded_NeverFail);

      if (pRes.GetAcquireResult() == ezResourceAcquireResult::Final)
        pSurf = pRes.GetPointer();
    }
  }

  return 1; // the "<Default>" ground type that is not "<None>"
}

static void QueryInputGeo(const ezNavmeshGeoWorldModuleInterface* pGeo, ezUInt32 uiCollisionLayer, ezBoundingBox bounds, ezAiNavMeshInputGeo& out_inputGeo)
{
  bounds.Grow(ezVec3(1.0f));

  ezHybridArray<ezNavmeshTriangle, 64> triangles;

  pGeo->RetrieveGeometryInArea(uiCollisionLayer, bounds, triangles);

  // sort all triangles by surface (pointer)
  triangles.Sort([](const ezNavmeshTriangle& lhs, const ezNavmeshTriangle& rhs)
    { return lhs.m_pSurface < rhs.m_pSurface; });

  const ezSurfaceResource* pPrevSurf = nullptr;
  ezInt8 iGroundType = 1; // the "<Default>" ground type that is not "<None>"

  for (ezUInt32 tri = 0; tri < triangles.GetCount(); ++tri)
  {
    if (triangles[tri].m_pSurface != pPrevSurf)
    {
      pPrevSurf = triangles[tri].m_pSurface;

      iGroundType = GetSurfaceGroundType(pPrevSurf);
      EZ_ASSERT_DEV(iGroundType < 32, "Area ID is out of range");
    }

    // we abuse the surface pointer to store the ground type int, so that we don't need any additional array and sorting logic
    triangles[tri].m_pSurface = reinterpret_cast<const ezSurfaceResource*>(iGroundType);
  }

  // sort all triangles by ground type (we wrote the ground type ID into the surface pointer above)
  // this means triangles with ground type 0 will be first, and higher IDs will come later -> should rasterize them in that deterministic order
  // and if several triangles are in the same spot, the higher ground ID should win
  triangles.Sort([](const ezNavmeshTriangle& lhs, const ezNavmeshTriangle& rhs)
    { return lhs.m_pSurface < rhs.m_pSurface; });

  out_inputGeo.m_Vertices.SetCount(triangles.GetCount() * 3);
  out_inputGeo.m_Triangles.SetCount(triangles.GetCount());
  out_inputGeo.m_TriangleAreaIDs.SetCount(triangles.GetCount());

  for (ezUInt32 tri = 0; tri < triangles.GetCount(); ++tri)
  {
    ezVec3& v1 = out_inputGeo.m_Vertices[(tri * 3) + 0];
    ezVec3& v2 = out_inputGeo.m_Vertices[(tri * 3) + 1];
    ezVec3& v3 = out_inputGeo.m_Vertices[(tri * 3) + 2];

    // NOTE: inverting the triangle order here ! Recast seems to use a different winding
    v1 = triangles[tri].m_Vertices[0];
    v2 = triangles[tri].m_Vertices[2];
    v3 = triangles[tri].m_Vertices[1];

    // convert from ez convention (Z up) to recast convention (Y up)
    ezMath::Swap(v1.y, v1.z);
    ezMath::Swap(v2.y, v2.z);
    ezMath::Swap(v3.y, v3.z);

    out_inputGeo.m_Triangles[tri].m_VertexIdx[0] = (tri * 3) + 0;
    out_inputGeo.m_Triangles[tri].m_VertexIdx[1] = (tri * 3) + 1;
    out_inputGeo.m_Triangles[tri].m_VertexIdx[2] = (tri * 3) + 2;

    out_inputGeo.m_TriangleAreaIDs[tri] = static_cast<ezUInt8>(reinterpret_cast<uintptr_t>(triangles[tri].m_pSurface));
  }
}

void ezAiNavMesh::BuildSector(SectorID sectorID, const ezNavmeshGeoWorldModuleInterface* pGeo)
{
  const ezVec2I32 sectorCoord = CalculateSectorCoord(sectorID);
  auto& sector = m_Sectors[sectorID];

  EZ_ASSERT_DEV(sector.m_FlagUpdateAvailable == 0, "Shouldn't update a sector that is already being updated");

  const ezBoundingBox bounds = GetSectorBounds(sectorCoord, -1000, +1000);

  ezAiNavMeshInputGeo inputGeo;
  {
    ezBoundingBox boundsWithBorder = bounds;
    const float cs = m_NavmeshConfig.m_fCellSize;
    const float borderSize = (int)ceilf(m_NavmeshConfig.m_fAgentRadius / cs) + 3;
    boundsWithBorder.m_vMin.x -= borderSize * cs;
    boundsWithBorder.m_vMin.y -= borderSize * cs;
    boundsWithBorder.m_vMax.x += borderSize * cs;
    boundsWithBorder.m_vMax.y += borderSize * cs;
    QueryInputGeo(pGeo, m_NavmeshConfig.m_uiCollisionLayer, boundsWithBorder, inputGeo);
  }
  if (!inputGeo.m_Vertices.IsEmpty())
  {
    rcContext recastContext;
    rcPolyMesh polyMesh;

    BuildRecastPolyMesh(m_NavmeshConfig, bounds, polyMesh, &recastContext, inputGeo.m_Vertices, inputGeo.m_Triangles, inputGeo.m_TriangleAreaIDs).AssertSuccess();

    if (polyMesh.nverts > 0 && polyMesh.npolys > 0)
    {
      BuildDetourNavMeshData(m_NavmeshConfig, polyMesh, sector.m_NavmeshDataNew, sectorCoord).AssertSuccess();
    }
  }

  {
    EZ_ASSERT_DEV(sector.m_FlagUpdateAvailable == 0, "Race condition in navmesh sector update");
    sector.m_FlagUpdateAvailable = 1;

    EZ_LOCK(m_Mutex);
    m_UpdatingSectors.PushBack(sectorID);
  }
}
