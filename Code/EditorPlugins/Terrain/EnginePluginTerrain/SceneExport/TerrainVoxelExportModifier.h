#pragma once

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginTerrain/EnginePluginTerrainDLL.h>

/// Scene export modifier that bakes voxel volume colliders to disk.
///
/// For each active ezTerrainVolumeComponent with EnableCollider = true, it reads back
/// the GPU-baked mesh data, cooks a Jolt triangle mesh, and writes a .ezBinJoltTriangleMesh file to
/// AssetCache/Generated/. A child game object named "VoxelCollider" is then created (or
/// reused) with an ezJoltStaticActorComponent pointing to that file.
class EZ_ENGINEPLUGINTERRAIN_DLL ezSceneExportModifier_TerrainVoxelCollision : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_TerrainVoxelCollision, ezSceneExportModifier);

public:
  virtual void ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport) override;
};
