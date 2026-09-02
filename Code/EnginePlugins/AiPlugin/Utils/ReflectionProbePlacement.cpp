#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Utils/RecastBuilder.h>
#include <AiPlugin/Utils/ReflectionProbePlacement.h>
#include <Foundation/Utilities/Progress.h>
#include <Recast.h>

#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

namespace
{
  /// Forwards Recast's own diagnostics into the ez log, which is where the failure reasons show up.
  class ezRecastLogContext : public rcContext
  {
  public:
    ezRecastLogContext()
      : rcContext(true)
    {
    }

  protected:
    virtual void doLog(const rcLogCategory category, const char* msg, const int len) override
    {
      if (category == RC_LOG_ERROR)
        ezLog::Error("Recast: {}", ezStringView(msg, msg + len));
      else
        ezLog::Debug("Recast: {}", ezStringView(msg, msg + len));
    }
  };
} // namespace

void ezReflectionProbePlacementSettings::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_Bounds.m_vMin;
  inout_stream << m_Bounds.m_vMax;
  inout_stream << m_fCellSize;
  inout_stream << m_fProbeHeight;
  inout_stream << m_fMinAreaSize;
  inout_stream << m_fLargeAreaRadius;
  inout_stream << m_fMergeDistance;
  inout_stream << m_uiMaxProbes;
  inout_stream << m_fMaxProbeSize;
  inout_stream << m_fMaxCeilingHeight;
  inout_stream << m_fOutdoorSpacing;
  inout_stream << m_uiMode;
  inout_stream << m_bGenerateBoxProbes;
}

void ezReflectionProbePlacementSettings::Deserialize(ezStreamReader& inout_stream)
{
  ezVec3 vMin, vMax;
  inout_stream >> vMin;
  inout_stream >> vMax;
  m_Bounds = ezBoundingBox::MakeFromMinMax(vMin, vMax);
  inout_stream >> m_fCellSize;
  inout_stream >> m_fProbeHeight;
  inout_stream >> m_fMinAreaSize;
  inout_stream >> m_fLargeAreaRadius;
  inout_stream >> m_fMergeDistance;
  inout_stream >> m_uiMaxProbes;
  inout_stream >> m_fMaxProbeSize;
  inout_stream >> m_fMaxCeilingHeight;
  inout_stream >> m_fOutdoorSpacing;
  inout_stream >> m_uiMode;
  inout_stream >> m_bGenerateBoxProbes;
}

ezReflectionProbePlacer::ezReflectionProbePlacer() = default;
ezReflectionProbePlacer::~ezReflectionProbePlacer() = default;

ezResult ezReflectionProbePlacer::BuildInputGeometry(const ezWorldGeoExtractionUtil::MeshObjectList& objects, const ezReflectionProbePlacementSettings& settings)
{
  m_Vertices.Clear();
  m_Triangles.Clear();
  m_TriangleAreaIDs.Clear();

  // Grow the region a bit, so that walls right at the border still block the rasterization properly.
  ezBoundingBox clipBox = settings.m_Bounds;
  clipBox.Grow(ezVec3(settings.m_fCellSize * 4.0f));

  for (const auto& object : objects)
  {
    ezResourceLock<ezCpuMeshResource> pCpuMesh(object.m_hMeshResource, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pCpuMesh.GetAcquireResult() != ezResourceAcquireResult::Final)
    {
      ezLog::Warning("Reflection probe placement: failed to retrieve CPU mesh '{}'", object.m_hMeshResource.GetResourceID());
      continue;
    }

    const auto& mbDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();

    const ezUInt32 uiVertexCount = mbDesc.GetVertexCount();
    const ezUInt32 uiTriangleCount = mbDesc.GetPrimitiveCount();

    if (uiVertexCount == 0 || uiTriangleCount == 0)
      continue;

    const ezVec3* pPositions = mbDesc.GetPositionData().GetPtr();
    if (pPositions == nullptr)
      continue;

    // Transform the vertices into world space up front, so that the triangle loop below stays cheap.
    ezDynamicArray<ezVec3> worldPositions;
    worldPositions.SetCountUninitialized(uiVertexCount);

    for (ezUInt32 i = 0; i < uiVertexCount; ++i)
    {
      worldPositions[i] = object.m_GlobalTransform.TransformPosition(pPositions[i]);
    }

    const bool b32Bit = mbDesc.Uses32BitIndices();
    const ezUInt8* pIndices = mbDesc.GetIndexBufferData().GetPtr();
    if (pIndices == nullptr)
      continue;

    for (ezUInt32 tri = 0; tri < uiTriangleCount; ++tri)
    {
      ezUInt32 idx[3];

      if (b32Bit)
      {
        const ezUInt32* pTyped = reinterpret_cast<const ezUInt32*>(pIndices);
        idx[0] = pTyped[tri * 3 + 0];
        idx[1] = pTyped[tri * 3 + 1];
        idx[2] = pTyped[tri * 3 + 2];
      }
      else
      {
        const ezUInt16* pTyped = reinterpret_cast<const ezUInt16*>(pIndices);
        idx[0] = pTyped[tri * 3 + 0];
        idx[1] = pTyped[tri * 3 + 1];
        idx[2] = pTyped[tri * 3 + 2];
      }

      if (idx[0] >= uiVertexCount || idx[1] >= uiVertexCount || idx[2] >= uiVertexCount)
        continue;

      const ezVec3 v[3] = {worldPositions[idx[0]], worldPositions[idx[1]], worldPositions[idx[2]]};

      // Skip triangles that can't influence the region at all.
      const ezBoundingBox triBounds = ezBoundingBox::MakeFromPoints(v, 3, sizeof(ezVec3));
      if (!triBounds.Overlaps(clipBox))
        continue;

      const ezUInt32 uiBaseIdx = m_Vertices.GetCount();

      // Recast uses Y as the up axis and the opposite winding order, so swap Y/Z and flip the triangle.
      m_Vertices.PushBack(ezVec3(v[0].x, v[0].z, v[0].y));
      m_Vertices.PushBack(ezVec3(v[2].x, v[2].z, v[2].y));
      m_Vertices.PushBack(ezVec3(v[1].x, v[1].z, v[1].y));

      m_Triangles.PushBack(ezAiNavMeshTriangle(uiBaseIdx + 0, uiBaseIdx + 1, uiBaseIdx + 2));
      // Every triangle starts out walkable. rcClearUnwalkableTriangles only ever clears the ones that
      // are too steep, so anything starting at RC_NULL_AREA (which is 0) would stay unwalkable.
      m_TriangleAreaIDs.PushBack(RC_WALKABLE_AREA);
    }
  }

  if (m_Triangles.IsEmpty())
  {
    ezLog::Warning("Reflection probe placement: no geometry found inside the given volume.");
    return EZ_FAILURE;
  }

  BuildColumnGrid();

  ezLog::Info("Reflection probe placement: using {} triangles.", m_Triangles.GetCount());
  return EZ_SUCCESS;
}

void ezReflectionProbePlacer::BuildColumnGrid()
{
  m_Columns.Clear();

  if (m_Vertices.IsEmpty())
    return;

  // The vertices are in Recast space here, where X/Z span the ground plane and Y is the height.
  const ezBoundingBox bounds = ezBoundingBox::MakeFromPoints(m_Vertices.GetData(), m_Vertices.GetCount(), sizeof(ezVec3));

  m_vGridOrigin.Set(bounds.m_vMin.x, bounds.m_vMin.z);

  const float fWidth = ezMath::Max(1.0f, bounds.m_vMax.x - bounds.m_vMin.x);
  const float fDepth = ezMath::Max(1.0f, bounds.m_vMax.z - bounds.m_vMin.z);

  // A fixed cell count keeps the memory bounded no matter how large the level is.
  constexpr ezInt32 iMaxCells = 128;
  m_fColumnSize = ezMath::Max(1.0f, ezMath::Max(fWidth, fDepth) / iMaxCells);

  m_vGridSize.x = ezMath::Min(iMaxCells, (ezInt32)ezMath::Ceil(fWidth / m_fColumnSize) + 1);
  m_vGridSize.y = ezMath::Min(iMaxCells, (ezInt32)ezMath::Ceil(fDepth / m_fColumnSize) + 1);

  m_Columns.SetCount(m_vGridSize.x * m_vGridSize.y);

  for (ezUInt32 tri = 0; tri < m_Triangles.GetCount(); ++tri)
  {
    const ezAiNavMeshTriangle& t = m_Triangles[tri];

    const ezVec3& v0 = m_Vertices[t.m_VertexIdx[0]];
    const ezVec3& v1 = m_Vertices[t.m_VertexIdx[1]];
    const ezVec3& v2 = m_Vertices[t.m_VertexIdx[2]];

    const float fMinX = ezMath::Min(v0.x, v1.x, v2.x);
    const float fMaxX = ezMath::Max(v0.x, v1.x, v2.x);
    const float fMinZ = ezMath::Min(v0.z, v1.z, v2.z);
    const float fMaxZ = ezMath::Max(v0.z, v1.z, v2.z);

    const ezInt32 iX0 = ezMath::Clamp((ezInt32)((fMinX - m_vGridOrigin.x) / m_fColumnSize), 0, m_vGridSize.x - 1);
    const ezInt32 iX1 = ezMath::Clamp((ezInt32)((fMaxX - m_vGridOrigin.x) / m_fColumnSize), 0, m_vGridSize.x - 1);
    const ezInt32 iZ0 = ezMath::Clamp((ezInt32)((fMinZ - m_vGridOrigin.y) / m_fColumnSize), 0, m_vGridSize.y - 1);
    const ezInt32 iZ1 = ezMath::Clamp((ezInt32)((fMaxZ - m_vGridOrigin.y) / m_fColumnSize), 0, m_vGridSize.y - 1);

    for (ezInt32 z = iZ0; z <= iZ1; ++z)
    {
      for (ezInt32 x = iX0; x <= iX1; ++x)
      {
        m_Columns[z * m_vGridSize.x + x].m_Triangles.PushBack(tri);
      }
    }
  }
}

float ezReflectionProbePlacer::FindCeilingHeight(const ezVec3& vPosition) const
{
  constexpr float fNoCeiling = 100000.0f;

  if (m_Columns.IsEmpty())
    return fNoCeiling;

  // vPosition arrives in ez space, the grid is in Recast space.
  const float fX = vPosition.x;
  const float fZ = vPosition.y;
  const float fY = vPosition.z;

  const ezInt32 iX = (ezInt32)((fX - m_vGridOrigin.x) / m_fColumnSize);
  const ezInt32 iZ = (ezInt32)((fZ - m_vGridOrigin.y) / m_fColumnSize);

  if (iX < 0 || iX >= m_vGridSize.x || iZ < 0 || iZ >= m_vGridSize.y)
    return fNoCeiling;

  float fClosest = fNoCeiling;

  for (ezUInt32 tri : m_Columns[iZ * m_vGridSize.x + iX].m_Triangles)
  {
    const ezAiNavMeshTriangle& t = m_Triangles[tri];

    const ezVec3& v0 = m_Vertices[t.m_VertexIdx[0]];
    const ezVec3& v1 = m_Vertices[t.m_VertexIdx[1]];
    const ezVec3& v2 = m_Vertices[t.m_VertexIdx[2]];

    // Barycentric test of the point against the triangle, projected onto the ground plane.
    const float fDenom = (v1.z - v2.z) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.z - v2.z);

    if (ezMath::Abs(fDenom) < ezMath::SmallEpsilon<float>())
      continue;

    const float fA = ((v1.z - v2.z) * (fX - v2.x) + (v2.x - v1.x) * (fZ - v2.z)) / fDenom;
    const float fB = ((v2.z - v0.z) * (fX - v2.x) + (v0.x - v2.x) * (fZ - v2.z)) / fDenom;
    const float fC = 1.0f - fA - fB;

    if (fA < 0.0f || fB < 0.0f || fC < 0.0f)
      continue;

    const float fHeight = fA * v0.y + fB * v1.y + fC * v2.y;

    // Only what is actually above the probe counts, with a little slack so the floor it stands on
    // is never mistaken for a ceiling.
    if (fHeight > fY + 0.5f)
    {
      fClosest = ezMath::Min(fClosest, fHeight - fY);
    }
  }

  return fClosest;
}

ezResult ezReflectionProbePlacer::GatherCandidates(const ezReflectionProbePlacementSettings& settings, float fAgentRadius, bool bLargeArea, ezDynamicArray<Candidate>& out_candidates)
{
  ezAiNavmeshConfig cfg;
  cfg.m_fCellSize = settings.m_fCellSize;
  cfg.m_fCellHeight = ezMath::Max(0.1f, settings.m_fCellSize * 0.5f);
  cfg.m_fAgentRadius = fAgentRadius;
  // A large agent needs head room as well, otherwise low corridors would still be reported as open areas.
  cfg.m_fAgentHeight = bLargeArea ? ezMath::Max(2.0f, settings.m_fProbeHeight + 0.5f) : 1.5f;
  cfg.m_fAgentStepHeight = 0.5f;
  cfg.m_WalkableSlope = ezAngle::MakeFromDegree(45);
  // Merge aggressively - we want few large regions, not a detailed navmesh.
  cfg.m_fMinRegionSize = ezMath::Sqrt(ezMath::Max(1.0f, settings.m_fMinAreaSize));
  cfg.m_fRegionMergeSize = 20.0f;
  cfg.m_fMaxEdgeLength = 12.0f;
  cfg.m_fMaxSimplificationError = 1.5f;
  // The detail mesh is only needed for accurate height queries, which we don't do.
  cfg.m_fDetailMeshSampleDistanceFactor = 0.0f;
  cfg.m_fDetailMeshSampleErrorFactor = 1.0f;

  ezRecastLogContext context;
  rcPolyMesh polyMesh;

  if (BuildRecastPolyMesh(cfg, settings.m_Bounds, polyMesh, &context, m_Vertices, m_Triangles, m_TriangleAreaIDs).Failed())
    return EZ_FAILURE;

  ezLog::Info("Reflection probe placement: recast pass (radius {}) produced {} polygons, {} vertices.", fAgentRadius, polyMesh.npolys, polyMesh.nverts);

  if (polyMesh.npolys <= 0 || polyMesh.nverts <= 0)
    return EZ_SUCCESS;

  const int nvp = polyMesh.nvp;
  ezUInt32 uiTooSmall = 0;
  ezUInt32 uiOutside = 0;
  ezUInt32 uiOutdoors = 0;

  for (int poly = 0; poly < polyMesh.npolys; ++poly)
  {
    if (polyMesh.areas[poly] == RC_NULL_AREA)
      continue;

    const unsigned short* p = &polyMesh.polys[poly * 2 * nvp];

    // Gather the polygon's corners, converting them back from Recast space into ez space.
    ezHybridArray<ezVec3, 8> corners;
    bool bTouchesBorder = false;

    for (int j = 0; j < nvp; ++j)
    {
      if (p[j] == RC_MESH_NULL_IDX)
        break;

      // An edge without a neighbor means the polygon ends there, either at a wall or at the tile border.
      if (p[nvp + j] == RC_MESH_NULL_IDX)
        bTouchesBorder = true;

      const unsigned short* v = &polyMesh.verts[p[j] * 3];

      // Recast stores vertices as grid indices relative to the mesh origin, in its own Y-up space.
      const float x = polyMesh.bmin[0] + v[0] * polyMesh.cs;
      const float y = polyMesh.bmin[1] + v[1] * polyMesh.ch;
      const float z = polyMesh.bmin[2] + v[2] * polyMesh.cs;

      corners.PushBack(ezVec3(x, z, y));
    }

    if (corners.GetCount() < 3)
      continue;

    // Area and centroid of the polygon, using the standard shoelace formula on the ground plane.
    float fArea = 0.0f;
    ezVec2 vCentroid = ezVec2::MakeZero();

    for (ezUInt32 i = 0; i < corners.GetCount(); ++i)
    {
      const ezVec2 a = corners[i].GetAsVec2();
      const ezVec2 b = corners[(i + 1) % corners.GetCount()].GetAsVec2();

      const float fCross = a.x * b.y - b.x * a.y;
      fArea += fCross;
      vCentroid += (a + b) * fCross;
    }

    fArea *= 0.5f;

    if (ezMath::Abs(fArea) < ezMath::SmallEpsilon<float>())
      continue;

    vCentroid /= (6.0f * fArea);
    fArea = ezMath::Abs(fArea);

    if (fArea < settings.m_fMinAreaSize)
    {
      ++uiTooSmall;
      continue;
    }

    ezBoundingBox polyBounds = ezBoundingBox::MakeFromPoints(corners.GetData(), corners.GetCount(), sizeof(ezVec3));

    // The centroid of a concave polygon can land outside of it. Falling back to the bounding box center
    // is not perfect either, but keeps the probe inside the general area.
    ezVec3 vPos(vCentroid.x, vCentroid.y, polyBounds.m_vMin.z);
    if (!polyBounds.Contains(vPos))
    {
      vPos = polyBounds.GetCenter();
      vPos.z = polyBounds.m_vMin.z;
    }

    if (!settings.m_Bounds.Contains(vPos))
    {
      ++uiOutside;
      continue;
    }

    const float fCeiling = FindCeilingHeight(vPos + ezVec3(0, 0, settings.m_fProbeHeight));
    const bool bIndoors = fCeiling <= settings.m_fMaxCeilingHeight;

    if (bIndoors)
    {
      if (settings.m_uiMode == ezReflectionProbePlacementMode::Outdoors)
        continue;
    }
    else
    {
      ++uiOutdoors;

      if (settings.m_uiMode == ezReflectionProbePlacementMode::Indoors)
        continue;
    }

    auto& candidate = out_candidates.ExpandAndGetRef();
    candidate.m_vPosition = vPos;
    candidate.m_vHalfSize = (polyBounds.GetHalfExtents()).GetAsVec2();
    candidate.m_fArea = fArea;
    candidate.m_fCeiling = fCeiling;
    candidate.m_bLargeArea = bLargeArea;
    // A polygon whose edges are all shared with neighbors lies in the middle of a bigger area, so the
    // "enclosed" classification only makes sense for those that actually end at geometry.
    candidate.m_bEnclosed = bTouchesBorder;
    candidate.m_bIndoors = bIndoors;
  }

  ezLog::Info("Reflection probe placement: {} candidates, {} too small, {} outside the volume, {} open to the sky.", out_candidates.GetCount(), uiTooSmall, uiOutside, uiOutdoors);

  return EZ_SUCCESS;
}

void ezReflectionProbePlacer::MergeCandidates(const ezReflectionProbePlacementSettings& settings, ezDynamicArray<Candidate>& inout_candidates)
{
  if (inout_candidates.IsEmpty())
    return;

  // Bigger areas win, so that a large room provides the probe and nearby small areas get absorbed into it.
  inout_candidates.Sort([](const Candidate& lhs, const Candidate& rhs)
    { return lhs.m_fArea > rhs.m_fArea; });

  ezDynamicArray<Candidate> merged;

  for (const Candidate& candidate : inout_candidates)
  {
    bool bAbsorbed = false;

    for (Candidate& existing : merged)
    {
      // Only merge within the same height band, otherwise probes of stacked floors collapse into one.
      if (ezMath::Abs(existing.m_vPosition.z - candidate.m_vPosition.z) > settings.m_fProbeHeight)
        continue;

      // Indoors and outdoors are different kinds of space: a probe standing in the open should not
      // swallow a candidate inside a nearby building, or it would reflect the sky onto its walls.
      if (existing.m_bIndoors != candidate.m_bIndoors)
        continue;

      const float fDist = (existing.m_vPosition.GetAsVec2() - candidate.m_vPosition.GetAsVec2()).GetLength();

      // Outdoors there is little to reflect nearby and a probe stays plausible over a much wider area,
      // so few, widely spaced probes are preferable to many small ones.
      const float fBaseReach = candidate.m_bIndoors ? settings.m_fMergeDistance : settings.m_fOutdoorSpacing;

      // Large probes cover more ground, so they may absorb candidates from further away.
      const float fReach = fBaseReach + ezMath::Min(existing.m_vHalfSize.x, existing.m_vHalfSize.y);

      if (fDist < fReach)
      {
        // Grow the survivor so that it still covers what it just absorbed, but never past the limit:
        // a probe that spans half the level reflects geometry that is nowhere near what it is applied to.
        const ezVec2 vOffset(ezMath::Abs(candidate.m_vPosition.x - existing.m_vPosition.x), ezMath::Abs(candidate.m_vPosition.y - existing.m_vPosition.y));
        existing.m_vHalfSize.x = ezMath::Min(settings.m_fMaxProbeSize, ezMath::Max(existing.m_vHalfSize.x, vOffset.x + candidate.m_vHalfSize.x));
        existing.m_vHalfSize.y = ezMath::Min(settings.m_fMaxProbeSize, ezMath::Max(existing.m_vHalfSize.y, vOffset.y + candidate.m_vHalfSize.y));
        existing.m_fArea += candidate.m_fArea;

        bAbsorbed = true;
        break;
      }
    }

    if (!bAbsorbed)
    {
      merged.PushBack(candidate);
    }
  }

  inout_candidates = std::move(merged);
}

ezResult ezReflectionProbePlacer::Compute(const ezReflectionProbePlacementSettings& settings, ezProgress& ref_progress)
{
  m_Results.Clear();

  if (m_Triangles.IsEmpty())
    return EZ_FAILURE;

  const bool bDoLargePass = settings.m_fLargeAreaRadius > 0.01f;

  ezProgressRange progress("Placing Reflection Probes", bDoLargePass ? 3 : 2, true, &ref_progress);

  // Pass 1: a small character, to find every spot that is reachable at all.
  progress.BeginNextStep("Analyzing walkable areas");

  ezDynamicArray<Candidate> candidates;
  if (GatherCandidates(settings, 0.3f, false, candidates).Failed())
    return EZ_FAILURE;

  if (!progress.BeginNextStep("Analyzing open areas"))
    return EZ_FAILURE;

  // Pass 2: a much larger character, which only fits into wide open areas. Those get bigger probes.
  ezDynamicArray<Candidate> largeCandidates;
  if (bDoLargePass)
  {
    if (GatherCandidates(settings, settings.m_fLargeAreaRadius, true, largeCandidates).Failed())
    {
      // A failed large pass is not fatal, we simply lose the size classification.
      largeCandidates.Clear();
    }

    // Mark the candidates from the first pass that also exist as an open area.
    for (Candidate& candidate : candidates)
    {
      for (const Candidate& large : largeCandidates)
      {
        if (ezMath::Abs(large.m_vPosition.z - candidate.m_vPosition.z) > settings.m_fProbeHeight)
          continue;

        const float fDist = (large.m_vPosition.GetAsVec2() - candidate.m_vPosition.GetAsVec2()).GetLength();
        if (fDist < ezMath::Max(large.m_vHalfSize.x, large.m_vHalfSize.y) + settings.m_fCellSize)
        {
          candidate.m_bLargeArea = true;
          break;
        }
      }
    }

    if (!progress.BeginNextStep("Merging probes"))
      return EZ_FAILURE;
  }

  MergeCandidates(settings, candidates);

  // Keep only the biggest areas if the budget is exceeded.
  candidates.Sort([](const Candidate& lhs, const Candidate& rhs)
    { return lhs.m_fArea > rhs.m_fArea; });

  if (candidates.GetCount() > settings.m_uiMaxProbes)
  {
    candidates.SetCount(settings.m_uiMaxProbes);
  }

  for (const Candidate& candidate : candidates)
  {
    auto& result = m_Results.ExpandAndGetRef();

    result.m_vPosition = candidate.m_vPosition;
    result.m_vPosition.z += settings.m_fProbeHeight;

    // The box should span from the floor to just under the ceiling that was measured for this spot,
    // so that its projection lines up with the actual room rather than an assumed height.
    const float fHeight = ezMath::Clamp(settings.m_fProbeHeight + candidate.m_fCeiling, 2.0f, settings.m_fMaxCeilingHeight + settings.m_fProbeHeight);

    // A box probe only makes sense where the area is actually bounded by geometry. In open areas the box
    // projection would map the reflection onto walls that aren't there.
    const bool bUseBox = settings.m_bGenerateBoxProbes && candidate.m_bIndoors && candidate.m_bEnclosed && !candidate.m_bLargeArea;

    if (bUseBox)
    {
      result.m_bIsBox = true;
      result.m_vExtents.x = ezMath::Clamp(candidate.m_vHalfSize.x * 2.0f, 2.0f, settings.m_fMaxProbeSize * 2.0f);
      result.m_vExtents.y = ezMath::Clamp(candidate.m_vHalfSize.y * 2.0f, 2.0f, settings.m_fMaxProbeSize * 2.0f);
      result.m_vExtents.z = fHeight;
    }
    else
    {
      // Reaching into the corners would mean using the diagonal, which over-inflates long thin areas.
      // The larger half extent plus a margin covers the area well enough and keeps the probe local.
      float fRadius = ezMath::Max(candidate.m_vHalfSize.x, candidate.m_vHalfSize.y) * 1.2f;

      if (!candidate.m_bIndoors)
      {
        // Outdoors the probes are spread out, so each one has to reach far enough to meet its
        // neighbours - otherwise the gaps between them get no reflection at all.
        fRadius = ezMath::Max(fRadius, settings.m_fOutdoorSpacing * 0.75f);
      }

      fRadius = ezMath::Clamp(fRadius, 2.0f, ezMath::Max(settings.m_fMaxProbeSize, candidate.m_bIndoors ? 0.0f : settings.m_fOutdoorSpacing));

      result.m_bIsBox = false;
      result.m_vExtents.Set(fRadius);
    }
  }

  ezLog::Info("Reflection probe placement: generated {} probes from {} candidate areas.", m_Results.GetCount(), candidates.GetCount());

  return EZ_SUCCESS;
}
