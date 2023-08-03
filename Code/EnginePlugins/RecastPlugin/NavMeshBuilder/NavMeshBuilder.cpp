#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <Recast/DetourNavMesh.h>
#include <Recast/DetourNavMeshBuilder.h>
#include <Recast/Recast.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezRecastConfig, ezNoBase, 1, ezRTTIDefaultAllocator<ezRecastConfig>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("AgentHeight", m_fAgentHeight)->AddAttributes(new ezDefaultValueAttribute(1.5f)),
    EZ_MEMBER_PROPERTY("AgentRadius", m_fAgentRadius)->AddAttributes(new ezDefaultValueAttribute(0.3f)),
    EZ_MEMBER_PROPERTY("AgentClimbHeight", m_fAgentClimbHeight)->AddAttributes(new ezDefaultValueAttribute(0.4f)),
    EZ_MEMBER_PROPERTY("WalkableSlope", m_WalkableSlope)->AddAttributes(new ezDefaultValueAttribute(ezAngle::MakeFromDegree(45))),
    EZ_MEMBER_PROPERTY("CellSize", m_fCellSize)->AddAttributes(new ezDefaultValueAttribute(0.2f)),
    EZ_MEMBER_PROPERTY("CellHeight", m_fCellHeight)->AddAttributes(new ezDefaultValueAttribute(0.2f)),
    EZ_MEMBER_PROPERTY("MinRegionSize", m_fMinRegionSize)->AddAttributes(new ezDefaultValueAttribute(3.0f)),
    EZ_MEMBER_PROPERTY("RegionMergeSize", m_fRegionMergeSize)->AddAttributes(new ezDefaultValueAttribute(20.0f)),
    EZ_MEMBER_PROPERTY("SampleDistanceFactor", m_fDetailMeshSampleDistanceFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("SampleErrorFactor", m_fDetailMeshSampleErrorFactor)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("MaxSimplification", m_fMaxSimplificationError)->AddAttributes(new ezDefaultValueAttribute(1.3f)),
    EZ_MEMBER_PROPERTY("MaxEdgeLength", m_fMaxEdgeLength)->AddAttributes(new ezDefaultValueAttribute(4.0f)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

class ezRcBuildContext : public rcContext
{
public:
  ezRcBuildContext() = default;

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

ezRecastNavMeshBuilder::ezRecastNavMeshBuilder() = default;
ezRecastNavMeshBuilder::~ezRecastNavMeshBuilder() = default;

void ezRecastNavMeshBuilder::Clear()
{
  m_BoundingBox = ezBoundingBox::MakeInvalid();
  m_Vertices.Clear();
  m_Triangles.Clear();
  m_TriangleAreaIDs.Clear();
  m_pRecastContext = nullptr;
}

ezResult ezRecastNavMeshBuilder::ExtractWorldGeometry(const ezWorld& world, ezWorldGeoExtractionUtil::MeshObjectList& out_worldGeo)
{
  ezWorldGeoExtractionUtil::ExtractWorldGeometry(out_worldGeo, world, ezWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration);

  return EZ_SUCCESS;
}

ezResult ezRecastNavMeshBuilder::Build(const ezRecastConfig& config, const ezWorldGeoExtractionUtil::MeshObjectList& geo,
  ezRecastNavMeshResourceDescriptor& out_navMeshDesc, ezProgress& ref_progress)
{
  EZ_LOG_BLOCK("ezRecastNavMeshBuilder::Build");

  ezProgressRange pg("Generating NavMesh", 4, true, &ref_progress);
  pg.SetStepWeighting(0, 0.1f);
  pg.SetStepWeighting(1, 0.1f);
  pg.SetStepWeighting(2, 0.6f);
  pg.SetStepWeighting(3, 0.2f);

  Clear();
  out_navMeshDesc.Clear();

  ezUniquePtr<ezRcBuildContext> recastContext = EZ_DEFAULT_NEW(ezRcBuildContext);
  m_pRecastContext = recastContext.Borrow();

  if (!pg.BeginNextStep("Triangulate Mesh"))
    return EZ_FAILURE;

  GenerateTriangleMeshFromDescription(geo);

  if (m_Vertices.IsEmpty())
  {
    ezLog::Debug("Navmesh is empty");
    return EZ_SUCCESS;
  }

  if (!pg.BeginNextStep("Compute AABB"))
    return EZ_FAILURE;

  ComputeBoundingBox();

  if (!pg.BeginNextStep("Build Poly Mesh"))
    return EZ_FAILURE;

  out_navMeshDesc.m_pNavMeshPolygons = EZ_DEFAULT_NEW(rcPolyMesh);

  if (BuildRecastPolyMesh(config, *out_navMeshDesc.m_pNavMeshPolygons, ref_progress).Failed())
    return EZ_FAILURE;

  if (!pg.BeginNextStep("Build NavMesh"))
    return EZ_FAILURE;

  if (BuildDetourNavMeshData(config, *out_navMeshDesc.m_pNavMeshPolygons, out_navMeshDesc.m_DetourNavmeshData).Failed())
    return EZ_FAILURE;

  return EZ_SUCCESS;
}

void ezRecastNavMeshBuilder::GenerateTriangleMeshFromDescription(const ezWorldGeoExtractionUtil::MeshObjectList& objects)
{
  EZ_LOG_BLOCK("ezRecastNavMeshBuilder::GenerateTriangleMesh");

  m_Triangles.Clear();
  m_TriangleAreaIDs.Clear();
  m_Vertices.Clear();


  ezUInt32 uiVertexOffset = 0;
  for (const ezWorldGeoExtractionUtil::MeshObject& object : objects)
  {
    ezResourceLock<ezCpuMeshResource> pCpuMesh(object.m_hMeshResource, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pCpuMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
    {
      continue;
    }

    const auto& meshBufferDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();

    m_Triangles.Reserve(m_Triangles.GetCount() + meshBufferDesc.GetPrimitiveCount());
    m_Vertices.Reserve(m_Vertices.GetCount() + meshBufferDesc.GetVertexCount());

    const ezVec3* pPositions = nullptr;
    ezUInt32 uiElementStride = 0;
    if (ezMeshBufferUtils::GetPositionStream(meshBufferDesc, pPositions, uiElementStride).Failed())
    {
      continue;
    }

    // convert from ez convention (Z up) to recast convention (Y up)
    ezMat3 m;
    m.SetRow(0, ezVec3(1, 0, 0));
    m.SetRow(1, ezVec3(0, 0, 1));
    m.SetRow(2, ezVec3(0, 1, 0));

    ezMat4 transform = ezMat4::MakeIdentity();
    transform.SetRotationalPart(m);
    transform = transform * object.m_GlobalTransform.GetAsMat4();

    // collect all vertices
    for (ezUInt32 i = 0; i < meshBufferDesc.GetVertexCount(); ++i)
    {
      ezVec3 pos = transform.TransformPosition(*pPositions);

      m_Vertices.PushBack(pos);

      pPositions = ezMemoryUtils::AddByteOffset(pPositions, uiElementStride);
    }

    // collect all indices
    bool flip = ezGraphicsUtils::IsTriangleFlipRequired(transform.GetRotationalPart());

    if (meshBufferDesc.HasIndexBuffer())
    {
      if (meshBufferDesc.Uses32BitIndices())
      {
        const ezUInt32* pTypedIndices = reinterpret_cast<const ezUInt32*>(meshBufferDesc.GetIndexBufferData().GetPtr());

        for (ezUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
        {
          auto& triangle = m_Triangles.ExpandAndGetRef();
          triangle.m_VertexIdx[0] = pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset;
          triangle.m_VertexIdx[1] = pTypedIndices[p * 3 + 1] + uiVertexOffset;
          triangle.m_VertexIdx[2] = pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset;
        }
      }
      else
      {
        const ezUInt16* pTypedIndices = reinterpret_cast<const ezUInt16*>(meshBufferDesc.GetIndexBufferData().GetPtr());

        for (ezUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
        {
          auto& triangle = m_Triangles.ExpandAndGetRef();
          triangle.m_VertexIdx[0] = pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset;
          triangle.m_VertexIdx[1] = pTypedIndices[p * 3 + 1] + uiVertexOffset;
          triangle.m_VertexIdx[2] = pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset;
        }
      }
    }
    else
    {
      ezUInt32 uiVertexIdx = uiVertexOffset;

      for (ezUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
      {
        auto& triangle = m_Triangles.ExpandAndGetRef();
        triangle.m_VertexIdx[0] = uiVertexIdx + 0;
        triangle.m_VertexIdx[1] = uiVertexIdx + (flip ? 2 : 1);
        triangle.m_VertexIdx[2] = uiVertexIdx + (flip ? 1 : 2);

        uiVertexIdx += 3;
      }
    }

    uiVertexOffset += meshBufferDesc.GetVertexCount();
  }

  // initialize the IDs to zero
  m_TriangleAreaIDs.SetCount(m_Triangles.GetCount());

  ezLog::Debug("Vertices: {0}, Triangles: {1}", m_Vertices.GetCount(), m_Triangles.GetCount());
}


void ezRecastNavMeshBuilder::ComputeBoundingBox()
{
  if (!m_Vertices.IsEmpty())
  {
    m_BoundingBox = ezBoundingBox::MakeFromPoints(m_Vertices.GetData(), m_Vertices.GetCount());
  }
}

void ezRecastNavMeshBuilder::FillOutConfig(rcConfig& cfg, const ezRecastConfig& config, const ezBoundingBox& bbox)
{
  ezMemoryUtils::ZeroFill(&cfg, 1);
  cfg.bmin[0] = bbox.m_vMin.x;
  cfg.bmin[1] = bbox.m_vMin.y;
  cfg.bmin[2] = bbox.m_vMin.z;
  cfg.bmax[0] = bbox.m_vMax.x;
  cfg.bmax[1] = bbox.m_vMax.y;
  cfg.bmax[2] = bbox.m_vMax.z;
  cfg.ch = config.m_fCellHeight;
  cfg.cs = config.m_fCellSize;
  cfg.walkableSlopeAngle = config.m_WalkableSlope.GetDegree();
  cfg.walkableHeight = (int)ceilf(config.m_fAgentHeight / cfg.ch);
  cfg.walkableClimb = (int)floorf(config.m_fAgentClimbHeight / cfg.ch);
  cfg.walkableRadius = (int)ceilf(config.m_fAgentRadius / cfg.cs);
  cfg.maxEdgeLen = (int)(config.m_fMaxEdgeLength / cfg.cs);
  cfg.maxSimplificationError = config.m_fMaxSimplificationError;
  cfg.minRegionArea = (int)ezMath::Square(config.m_fMinRegionSize);
  cfg.mergeRegionArea = (int)ezMath::Square(config.m_fRegionMergeSize);
  cfg.maxVertsPerPoly = 6;
  cfg.detailSampleDist = config.m_fDetailMeshSampleDistanceFactor < 0.9f ? 0 : cfg.cs * config.m_fDetailMeshSampleDistanceFactor;
  cfg.detailSampleMaxError = cfg.ch * config.m_fDetailMeshSampleErrorFactor;

  rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);
}

ezResult ezRecastNavMeshBuilder::BuildRecastPolyMesh(const ezRecastConfig& config, rcPolyMesh& out_PolyMesh, ezProgress& progress)
{
  ezProgressRange pgRange("Build Poly Mesh", 13, true, &progress);

  rcConfig cfg;
  FillOutConfig(cfg, config, m_BoundingBox);

  ezRcBuildContext* pContext = m_pRecastContext;
  const float* pVertices = &m_Vertices[0].x;
  const ezInt32* pTriangles = &m_Triangles[0].m_VertexIdx[0];

  rcHeightfield* heightfield = rcAllocHeightfield();
  EZ_SCOPE_EXIT(rcFreeHeightField(heightfield));

  if (!pgRange.BeginNextStep("Creating Heightfield"))
    return EZ_FAILURE;

  if (!rcCreateHeightfield(pContext, *heightfield, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
  {
    pContext->log(RC_LOG_ERROR, "Could not create solid heightfield");
    return EZ_FAILURE;
  }

  if (!pgRange.BeginNextStep("Mark Walkable Area"))
    return EZ_FAILURE;

  // TODO Instead of this, it should use area IDs and then clear the non-walkable triangles
  rcMarkWalkableTriangles(
    pContext, cfg.walkableSlopeAngle, pVertices, m_Vertices.GetCount(), pTriangles, m_Triangles.GetCount(), m_TriangleAreaIDs.GetData());

  if (!pgRange.BeginNextStep("Rasterize Triangles"))
    return EZ_FAILURE;

  if (!rcRasterizeTriangles(
        pContext, pVertices, m_Vertices.GetCount(), pTriangles, m_TriangleAreaIDs.GetData(), m_Triangles.GetCount(), *heightfield, cfg.walkableClimb))
  {
    pContext->log(RC_LOG_ERROR, "Could not rasterize triangles");
    return EZ_FAILURE;
  }

  // Optional stuff
  {
    if (!pgRange.BeginNextStep("Filter Low Hanging Obstacles"))
      return EZ_FAILURE;

    // if (m_filterLowHangingObstacles)
    rcFilterLowHangingWalkableObstacles(pContext, cfg.walkableClimb, *heightfield);

    if (!pgRange.BeginNextStep("Filter Ledge Spans"))
      return EZ_FAILURE;

    // if (m_filterLedgeSpans)
    rcFilterLedgeSpans(pContext, cfg.walkableHeight, cfg.walkableClimb, *heightfield);

    if (!pgRange.BeginNextStep("Filter Low Height Spans"))
      return EZ_FAILURE;

    // if (m_filterWalkableLowHeightSpans)
    rcFilterWalkableLowHeightSpans(pContext, cfg.walkableHeight, *heightfield);
  }

  if (!pgRange.BeginNextStep("Build Compact Heightfield"))
    return EZ_FAILURE;

  rcCompactHeightfield* compactHeightfield = rcAllocCompactHeightfield();
  EZ_SCOPE_EXIT(rcFreeCompactHeightfield(compactHeightfield));

  if (!rcBuildCompactHeightfield(pContext, cfg.walkableHeight, cfg.walkableClimb, *heightfield, *compactHeightfield))
  {
    pContext->log(RC_LOG_ERROR, "Could not build compact data");
    return EZ_FAILURE;
  }

  if (!pgRange.BeginNextStep("Erode Walkable Area"))
    return EZ_FAILURE;

  if (!rcErodeWalkableArea(pContext, cfg.walkableRadius, *compactHeightfield))
  {
    pContext->log(RC_LOG_ERROR, "Could not erode with character radius");
    return EZ_FAILURE;
  }

  // (Optional) Mark areas.
  //{
  //  const ConvexVolume* vols = m_geom->getConvexVolumes();
  //  for (int i = 0; i < m_geom->getConvexVolumeCount(); ++i)
  //    rcMarkConvexPolyArea(pContext, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area,
  //    *compactHeightfield);
  //}


  // Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
  // Default algorithm is 'Watershed'
  {
    // PARTITION_WATERSHED
    {
      if (!pgRange.BeginNextStep("Build Distance Field"))
        return EZ_FAILURE;

      // Prepare for region partitioning, by calculating distance field along the walkable surface.
      if (!rcBuildDistanceField(pContext, *compactHeightfield))
      {
        pContext->log(RC_LOG_ERROR, "Could not build distance field.");
        return EZ_FAILURE;
      }

      if (!pgRange.BeginNextStep("Build Regions"))
        return EZ_FAILURE;

      // Partition the walkable surface into simple regions without holes.
      if (!rcBuildRegions(pContext, *compactHeightfield, 0, cfg.minRegionArea, cfg.mergeRegionArea))
      {
        pContext->log(RC_LOG_ERROR, "Could not build watershed regions.");
        return EZ_FAILURE;
      }
    }

    //// PARTITION_MONOTONE
    //{
    //  // Partition the walkable surface into simple regions without holes.
    //  // Monotone partitioning does not need distance field.
    //  if (!rcBuildRegionsMonotone(pContext, *compactHeightfield, 0, cfg.minRegionArea, cfg.mergeRegionArea))
    //  {
    //    pContext->log(RC_LOG_ERROR, "Could not build monotone regions.");
    //    return EZ_FAILURE;
    //  }
    //}

    //// PARTITION_LAYERS
    //{
    //  // Partition the walkable surface into simple regions without holes.
    //  if (!rcBuildLayerRegions(pContext, *compactHeightfield, 0, cfg.minRegionArea))
    //  {
    //    pContext->log(RC_LOG_ERROR, "Could not build layer regions.");
    //    return EZ_FAILURE;
    //  }
    //}
  }

  if (!pgRange.BeginNextStep("Build Contours"))
    return EZ_FAILURE;

  rcContourSet* contourSet = rcAllocContourSet();
  EZ_SCOPE_EXIT(rcFreeContourSet(contourSet));

  if (!rcBuildContours(pContext, *compactHeightfield, cfg.maxSimplificationError, cfg.maxEdgeLen, *contourSet))
  {
    pContext->log(RC_LOG_ERROR, "Could not create contours");
    return EZ_FAILURE;
  }

  if (!pgRange.BeginNextStep("Build Poly Mesh"))
    return EZ_FAILURE;

  if (!rcBuildPolyMesh(pContext, *contourSet, cfg.maxVertsPerPoly, out_PolyMesh))
  {
    pContext->log(RC_LOG_ERROR, "Could not triangulate contours");
    return EZ_FAILURE;
  }

  //////////////////////////////////////////////////////////////////////////
  // Detour Navmesh

  if (!pgRange.BeginNextStep("Set Area Flags"))
    return EZ_FAILURE;

  // TODO modify area IDs and flags

  for (int i = 0; i < out_PolyMesh.npolys; ++i)
  {
    if (out_PolyMesh.areas[i] == RC_WALKABLE_AREA)
    {
      out_PolyMesh.flags[i] = 0xFFFF;
    }
  }

  return EZ_SUCCESS;
}

ezResult ezRecastNavMeshBuilder::BuildDetourNavMeshData(const ezRecastConfig& config, const rcPolyMesh& polyMesh, ezDataBuffer& NavmeshData)
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
  params.walkableClimb = config.m_fAgentClimbHeight;
  rcVcopy(params.bmin, polyMesh.bmin);
  rcVcopy(params.bmax, polyMesh.bmax);
  params.cs = config.m_fCellSize;
  params.ch = config.m_fCellHeight;
  params.buildBvTree = true;

  ezUInt8* navData = nullptr;
  ezInt32 navDataSize = 0;

  if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
  {
    ezLog::Error("Could not build Detour navmesh.");
    return EZ_FAILURE;
  }

  NavmeshData.SetCountUninitialized(navDataSize);
  ezMemoryUtils::Copy(NavmeshData.GetData(), navData, navDataSize);

  dtFree(navData);
  return EZ_SUCCESS;
}

ezResult ezRecastConfig::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(1);

  inout_stream << m_fAgentHeight;
  inout_stream << m_fAgentRadius;
  inout_stream << m_fAgentClimbHeight;
  inout_stream << m_WalkableSlope;
  inout_stream << m_fCellSize;
  inout_stream << m_fCellHeight;
  inout_stream << m_fMaxEdgeLength;
  inout_stream << m_fMaxSimplificationError;
  inout_stream << m_fMinRegionSize;
  inout_stream << m_fRegionMergeSize;
  inout_stream << m_fDetailMeshSampleDistanceFactor;
  inout_stream << m_fDetailMeshSampleErrorFactor;

  return EZ_SUCCESS;
}

ezResult ezRecastConfig::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream.ReadVersion(1);

  inout_stream >> m_fAgentHeight;
  inout_stream >> m_fAgentRadius;
  inout_stream >> m_fAgentClimbHeight;
  inout_stream >> m_WalkableSlope;
  inout_stream >> m_fCellSize;
  inout_stream >> m_fCellHeight;
  inout_stream >> m_fMaxEdgeLength;
  inout_stream >> m_fMaxSimplificationError;
  inout_stream >> m_fMinRegionSize;
  inout_stream >> m_fRegionMergeSize;
  inout_stream >> m_fDetailMeshSampleDistanceFactor;
  inout_stream >> m_fDetailMeshSampleErrorFactor;

  return EZ_SUCCESS;
}
