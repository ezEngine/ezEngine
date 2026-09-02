#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation/NavMesh.h>

struct rcConfig;
struct rcPolyMesh;
class rcContext;

/// Fills out a Recast configuration from an ezEngine navmesh configuration.
///
/// The bounding box is expected in ez coordinates (Z up) and is converted to Recast's convention internally.
EZ_AIPLUGIN_DLL void FillOutConfig(rcConfig& ref_cfg, const ezAiNavmeshConfig& config, const ezBoundingBox& bbox);

/// Rasterizes the given triangles and builds a Recast polygon mesh of the walkable areas.
///
/// The vertices must already be in Recast convention (Y up), which is what ezAiBuildNavmeshInputGeo produces.
/// The height range of the given bounding box is ignored and recomputed from the vertices.
EZ_AIPLUGIN_DLL ezResult BuildRecastPolyMesh(const ezAiNavmeshConfig& config, ezBoundingBox aabb, rcPolyMesh& out_polyMesh, rcContext* pContext, ezArrayPtr<const ezVec3> vertices, ezArrayPtr<const ezAiNavMeshTriangle> triangles, ezArrayPtr<ezUInt8> triangleAreaIDs);
