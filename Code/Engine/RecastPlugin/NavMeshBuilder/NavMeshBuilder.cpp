#include <PCH.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <ThirdParty/Recast/Recast.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Configuration/CVar.h>

class ezRcBuildContext : public rcContext
{
public:
  ezRcBuildContext() {}

protected:
  virtual void doLog(const rcLogCategory category, const char* msg, const int len)
  {
    switch (category)
    {
    case RC_LOG_ERROR:
      ezLog::Error("Recast: {0}", msg);
      return;
    case RC_LOG_WARNING:
      ezLog::Warning("Recast: {0}", msg);
      return;
    case RC_LOG_PROGRESS:
      ezLog::Debug("Recast: {0}", msg);
      return;

    default:
      ezLog::Error("Unknwon recast log: {0}", msg);
      return;
    }
  }
};

ezRecastNavMeshBuilder::ezRecastNavMeshBuilder() { }

ezRecastNavMeshBuilder::~ezRecastNavMeshBuilder()
{
  rcFreePolyMesh(m_polyMesh);
  rcFreePolyMeshDetail(m_detailMesh);
}

void ezRecastNavMeshBuilder::Build(const ezNavMeshDescription& desc)
{
  EZ_LOG_BLOCK("ezRecastNavMeshBuilder::Build");

  ezStopwatch watch;

  ezUniquePtr<ezRcBuildContext> recastContext = EZ_DEFAULT_NEW(ezRcBuildContext);
  m_pRecastContext = recastContext.Borrow();

  GenerateTriangleMeshFromDescription(desc);
  ComputeBoundingBox();

  ezLog::Debug("Generate Triangle Mesh: {0}ms", ezArgF(watch.Checkpoint().GetMilliseconds()));

  BuildRecastNavMesh();

  ezLog::Debug("Build Recast Nav Mesh: {0}ms", ezArgF(watch.Checkpoint().GetMilliseconds()));
}

void ezRecastNavMeshBuilder::ReserveMemory(const ezNavMeshDescription& desc)
{
  const ezUInt32 uiBoxes = desc.m_BoxObstacles.GetCount();
  const ezUInt32 uiBoxTriangles = uiBoxes * 12;
  const ezUInt32 uiBoxVertices = uiBoxes * 8;

  m_Triangles.Reserve(uiBoxTriangles);
  m_TriangleAreaIDs.Reserve(uiBoxTriangles);
  m_Vertices.Reserve(uiBoxVertices);
}

void ezRecastNavMeshBuilder::GenerateTriangleMeshFromDescription(const ezNavMeshDescription& desc)
{
  EZ_LOG_BLOCK("ezRecastNavMeshBuilder::GenerateTriangleMesh");

  m_Triangles.Clear();
  m_TriangleAreaIDs.Clear();
  m_Vertices.Clear();

  ReserveMemory(desc);

  for (const auto& box : desc.m_BoxObstacles)
  {
    const ezUInt32 uiFirstVtx = m_Vertices.GetCount();

    // add the 8 box vertices
    {
      ezVec3 ext = box.m_vHalfExtents;

      ezVec3 exts[8];
      exts[0] = ezVec3(ext.x, ext.y, ext.z);
      exts[1] = ezVec3(ext.x, ext.y, -ext.z);
      exts[2] = ezVec3(ext.x, -ext.y, ext.z);
      exts[3] = ezVec3(ext.x, -ext.y, -ext.z);
      exts[4] = ezVec3(-ext.x, ext.y, ext.z);
      exts[5] = ezVec3(-ext.x, ext.y, -ext.z);
      exts[6] = ezVec3(-ext.x, -ext.y, ext.z);
      exts[7] = ezVec3(-ext.x, -ext.y, -ext.z);

      for (ezUInt32 i = 0; i < 8; ++i)
      {
        ezVec3 pos = box.m_vPosition + box.m_qRotation * exts[i];
        ezMath::Swap(pos.y, pos.z);

        m_Vertices.ExpandAndGetRef() = pos;
      }
    }

    // Add all triangles
    {
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 0, uiFirstVtx + 5, uiFirstVtx + 1);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 0, uiFirstVtx + 4, uiFirstVtx + 5);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 2, uiFirstVtx + 1, uiFirstVtx + 3);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 2, uiFirstVtx + 0, uiFirstVtx + 1);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 6, uiFirstVtx + 3, uiFirstVtx + 7);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 6, uiFirstVtx + 2, uiFirstVtx + 3);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 4, uiFirstVtx + 7, uiFirstVtx + 5);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 4, uiFirstVtx + 6, uiFirstVtx + 7);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 4, uiFirstVtx + 2, uiFirstVtx + 6);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 4, uiFirstVtx + 0, uiFirstVtx + 2);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 7, uiFirstVtx + 1, uiFirstVtx + 5);
      m_Triangles.ExpandAndGetRef() = Triangle(uiFirstVtx + 7, uiFirstVtx + 3, uiFirstVtx + 1);
    }

    // initialize the IDs to zero
    m_TriangleAreaIDs.SetCount(m_Triangles.GetCount());
  }

  ezLog::Debug("Vertices: {0}, Triangles: {1}", m_Vertices.GetCount(), m_Triangles.GetCount());
}


void ezRecastNavMeshBuilder::ComputeBoundingBox()
{
  m_BoundingBox.SetFromPoints(m_Vertices.GetData(), m_Vertices.GetCount());
}

ezCVarInt g_CVarMinRegionArea("ai_MinRegionArea", 0, ezCVarFlags::Default, "");
ezCVarInt g_CVarMergeRegionArea("ai_MergeRegionArea", 0, ezCVarFlags::Default, "");

void ezRecastNavMeshBuilder::FillOutConfig(rcConfig& cfg)
{
  const float fAgentHeight = 1.5f;
  const float fAgentClimb = 0.9f;
  const float fAgentRadius = 0.2f;
  const float fMaxEdgeLen = 12.0f;
  const float fMaxWalkSlope = 45.0f;
  const float fMinRegionSize = 0.0f;//1
  const float fRegionMergeSize = 20.0f;//2
  const float detailSampleDist = 6.0f;

  ezMemoryUtils::ZeroFill(&cfg);
  cfg.bmin[0] = m_BoundingBox.m_vMin.x;
  cfg.bmin[1] = m_BoundingBox.m_vMin.y;
  cfg.bmin[2] = m_BoundingBox.m_vMin.z;
  cfg.bmax[0] = m_BoundingBox.m_vMax.x;
  cfg.bmax[1] = m_BoundingBox.m_vMax.y;
  cfg.bmax[2] = m_BoundingBox.m_vMax.z;
  cfg.ch = 0.2f;
  cfg.cs = 0.3f;
  cfg.walkableSlopeAngle = fMaxWalkSlope;
  cfg.walkableHeight = (int)ceilf(fAgentHeight / cfg.ch);
  cfg.walkableClimb = (int)floorf(fAgentClimb / cfg.ch);
  cfg.walkableRadius = (int)ceilf(fAgentRadius / cfg.cs);
  cfg.maxEdgeLen = (int)(fMaxEdgeLen / cfg.cs);
  cfg.maxSimplificationError = 1.3f;
  cfg.minRegionArea = g_CVarMinRegionArea;// (int)ezMath::Square(8);// fMinRegionSize / cfg.cs);
  cfg.mergeRegionArea = g_CVarMergeRegionArea;//(int)ezMath::Square(20);// fRegionMergeSize / cfg.cs);
  cfg.maxVertsPerPoly = 6;
  cfg.detailSampleDist = detailSampleDist < 0.9f ? 0 : cfg.cs * detailSampleDist;
  cfg.detailSampleMaxError = cfg.ch * 1.0f;

  rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);
}

void ezRecastNavMeshBuilder::BuildRecastNavMesh()
{
  rcConfig cfg;
  FillOutConfig(cfg);

  ezRcBuildContext* pContext = m_pRecastContext;
  const float* pVertices = &m_Vertices[0].x;
  const ezInt32* pTriangles = &m_Triangles[0].m_VertexIdx[0];

  rcHeightfield* heightfield = rcAllocHeightfield();
  EZ_SCOPE_EXIT(rcFreeHeightField(heightfield));

  if (!rcCreateHeightfield(pContext, *heightfield, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
  {
    pContext->log(RC_LOG_ERROR, "Could not create solid heightfield");
    return;
  }

  rcMarkWalkableTriangles(pContext, cfg.walkableSlopeAngle, pVertices, m_Vertices.GetCount(), pTriangles, m_Triangles.GetCount(), m_TriangleAreaIDs.GetData());

  if (!rcRasterizeTriangles(pContext, pVertices, m_Vertices.GetCount(), pTriangles, m_TriangleAreaIDs.GetData(), m_Triangles.GetCount(), *heightfield, cfg.walkableClimb))
  {
    pContext->log(RC_LOG_ERROR, "Could not rasterize triangles");
    return;
  }

  // Optional stuff
  {
    //if (m_filterLowHangingObstacles)
    rcFilterLowHangingWalkableObstacles(pContext, cfg.walkableClimb, *heightfield);

    //if (m_filterLedgeSpans)
    rcFilterLedgeSpans(pContext, cfg.walkableHeight, cfg.walkableClimb, *heightfield);

    //if (m_filterWalkableLowHeightSpans)
    rcFilterWalkableLowHeightSpans(pContext, cfg.walkableHeight, *heightfield);
  }

  rcCompactHeightfield* compactHeightfield = rcAllocCompactHeightfield();
  EZ_SCOPE_EXIT(rcFreeCompactHeightfield(compactHeightfield));

  if (!rcBuildCompactHeightfield(pContext, cfg.walkableHeight, cfg.walkableClimb, *heightfield, *compactHeightfield))
  {
    pContext->log(RC_LOG_ERROR, "Could not build compact data");
    return;
  }

  if (!rcErodeWalkableArea(pContext, cfg.walkableRadius, *compactHeightfield))
  {
    pContext->log(RC_LOG_ERROR, "Could not erode with character radius");
    return;
  }

  // (Optional) Mark areas.
  //{
  //  const ConvexVolume* vols = m_geom->getConvexVolumes();
  //  for (int i = 0; i < m_geom->getConvexVolumeCount(); ++i)
  //    rcMarkConvexPolyArea(pContext, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *compactHeightfield);
  //}


  // Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
  // Default algorithm is 'Watershed'
  {
    // PARTITION_WATERSHED
    {
      // Prepare for region partitioning, by calculating distance field along the walkable surface.
      if (!rcBuildDistanceField(pContext, *compactHeightfield))
      {
        pContext->log(RC_LOG_ERROR, "Could not build distance field.");
        return;
      }

      // Partition the walkable surface into simple regions without holes.
      if (!rcBuildRegions(pContext, *compactHeightfield, 0, cfg.minRegionArea, cfg.mergeRegionArea))
      {
        pContext->log(RC_LOG_ERROR, "Could not build watershed regions.");
        return;
      }
    }

    //// PARTITION_MONOTONE
    //{
    //  // Partition the walkable surface into simple regions without holes.
    //  // Monotone partitioning does not need distance field.
    //  if (!rcBuildRegionsMonotone(pContext, *compactHeightfield, 0, cfg.minRegionArea, cfg.mergeRegionArea))
    //  {
    //    pContext->log(RC_LOG_ERROR, "Could not build monotone regions.");
    //    return;
    //  }
    //}

    //// PARTITION_LAYERS
    //{
    //  // Partition the walkable surface into simple regions without holes.
    //  if (!rcBuildLayerRegions(pContext, *compactHeightfield, 0, cfg.minRegionArea))
    //  {
    //    pContext->log(RC_LOG_ERROR, "Could not build layer regions.");
    //    return;
    //  }
    //}
  }

  rcContourSet* contourSet = rcAllocContourSet();
  EZ_SCOPE_EXIT(rcFreeContourSet(contourSet));

  if (!rcBuildContours(pContext, *compactHeightfield, cfg.maxSimplificationError, cfg.maxEdgeLen, *contourSet))
  {
    pContext->log(RC_LOG_ERROR, "Could not create contours");
    return;
  }

  m_polyMesh = rcAllocPolyMesh();
  // this is not deallocated, it is the final result !

  if (!rcBuildPolyMesh(pContext, *contourSet, cfg.maxVertsPerPoly, *m_polyMesh))
  {
    pContext->log(RC_LOG_ERROR, "Could not triangulate contours");
    return;
  }

  m_detailMesh = rcAllocPolyMeshDetail();
  // this is not deallocated, it is the final result !

  if (!rcBuildPolyMeshDetail(pContext, *m_polyMesh, *compactHeightfield, cfg.detailSampleDist, cfg.detailSampleMaxError, *m_detailMesh))
  {
    pContext->log(RC_LOG_ERROR, "buildNavigation: Could not build detail mesh.");
    return;
  }

  ezLog::Success("Recast Nav Mesh generated.");
}


