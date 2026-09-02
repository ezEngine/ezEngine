#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Vec3.h>
#include <AiPlugin/Navigation/NavMesh.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

class ezProgress;

/// Which kind of space reflection probes should be placed in.
struct ezReflectionProbePlacementMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Indoors,  ///< Only where something is overhead. Open areas are left to the sky light.
    Outdoors, ///< Only where the sky is visible. Probes are spread out to cover large areas.
    Both,     ///< Everywhere that is walkable.

    Default = Both
  };
};

/// Settings for the automatic reflection probe placement. Mirrors ezReflectionProbePlacementComponent.
struct EZ_AIPLUGIN_DLL ezReflectionProbePlacementSettings
{
  ezBoundingBox m_Bounds = ezBoundingBox::MakeInvalid();
  float m_fCellSize = 0.5f;
  float m_fProbeHeight = 1.7f;
  float m_fMinAreaSize = 4.0f;
  float m_fLargeAreaRadius = 1.5f;
  float m_fMergeDistance = 4.0f;
  ezUInt32 m_uiMaxProbes = 64;
  float m_fMaxProbeSize = 12.0f;
  float m_fMaxCeilingHeight = 8.0f;
  float m_fOutdoorSpacing = 30.0f;
  ezUInt8 m_uiMode = ezReflectionProbePlacementMode::Both;
  bool m_bGenerateBoxProbes = true;

  void Serialize(ezStreamWriter& inout_stream) const;
  void Deserialize(ezStreamReader& inout_stream);
};

/// A single probe that the placement algorithm decided to create.
struct EZ_AIPLUGIN_DLL ezReflectionProbePlacementResult
{
  /// Center of the probe in world space.
  ezVec3 m_vPosition = ezVec3::MakeZero();

  /// Extents of a box probe, or (radius, radius, radius) for a sphere probe.
  ezVec3 m_vExtents = ezVec3(5.0f);

  /// Whether this is a box probe. Otherwise it is a sphere probe.
  bool m_bIsBox = false;
};

/// Analyzes the geometry of a world region and decides where to place reflection probes.
///
/// The analysis rasterizes the geometry into a Recast heightfield and extracts the walkable areas as
/// polygons. Each sufficiently large polygon becomes a probe, placed at its center at eye height.
/// A second pass with a larger character radius identifies wide open areas, which get larger probes.
///
/// The geometry has to be extracted from the world on the main thread first (see ezWorldGeoExtractionUtil),
/// the analysis itself can then run on a worker thread.
class EZ_AIPLUGIN_DLL ezReflectionProbePlacer
{
public:
  ezReflectionProbePlacer();
  ~ezReflectionProbePlacer();

  /// Converts the extracted mesh objects into a triangle soup that Recast can rasterize.
  ///
  /// This resolves the CPU mesh resources and therefore has to run where blocking resource loading is allowed.
  /// Triangles fully outside the settings' bounding box are discarded.
  ezResult BuildInputGeometry(const ezWorldGeoExtractionUtil::MeshObjectList& objects, const ezReflectionProbePlacementSettings& settings);

  /// Runs the analysis and computes the probe positions and sizes.
  ezResult Compute(const ezReflectionProbePlacementSettings& settings, ezProgress& ref_progress);

  ezArrayPtr<const ezReflectionProbePlacementResult> GetResults() const { return m_Results; }

private:
  struct Candidate
  {
    ezVec3 m_vPosition;
    ezVec2 m_vHalfSize;
    float m_fArea;
    float m_fCeiling;
    bool m_bLargeArea;
    bool m_bEnclosed;
    bool m_bIndoors;
  };

  ezResult GatherCandidates(const ezReflectionProbePlacementSettings& settings, float fAgentRadius, bool bLargeArea, ezDynamicArray<Candidate>& out_candidates);
  void MergeCandidates(const ezReflectionProbePlacementSettings& settings, ezDynamicArray<Candidate>& inout_candidates);

  /// Builds the grid that FindCeilingHeight() looks triangles up in.
  void BuildColumnGrid();

  /// Height of the closest triangle above the given point, or a very large value when there is none.
  ///
  /// Used to tell an indoor spot from one that is open to the sky. This is a downwards facing test only:
  /// it reports what is overhead, not how far away the walls are.
  float FindCeilingHeight(const ezVec3& vPosition) const;

  /// One cell of the lookup grid, holding the indices of the triangles overlapping it.
  struct Column
  {
    ezDynamicArray<ezUInt32> m_Triangles;
  };

  ezDynamicArray<Column> m_Columns;
  ezVec2 m_vGridOrigin = ezVec2::MakeZero();
  ezVec2I32 m_vGridSize = ezVec2I32::MakeZero();
  float m_fColumnSize = 2.0f;

  ezDynamicArray<ezVec3> m_Vertices;
  ezDynamicArray<ezAiNavMeshTriangle> m_Triangles;
  ezDynamicArray<ezUInt8> m_TriangleAreaIDs;
  ezDynamicArray<ezReflectionProbePlacementResult> m_Results;
};
