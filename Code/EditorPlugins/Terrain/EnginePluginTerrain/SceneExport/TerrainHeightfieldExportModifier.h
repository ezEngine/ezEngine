#pragma once

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginTerrain/EnginePluginTerrainDLL.h>

/// Scene export modifier that bakes terrain heightfield colliders to disk.
///
/// For each ezTerrainPatchComponent with a non-None collider mode, it reads back
/// the GPU-baked height data, cooks a Jolt heightfield shape, and writes a
/// .ezBinJoltHeightfield file to AssetCache/Generated/. A child game object named
/// "TerrainCollider" is then created (or reused) with an ezJoltHeightfieldColliderComponent pointing to that file.
class EZ_ENGINEPLUGINTERRAIN_DLL ezSceneExportModifier_TerrainHeightfieldCollision : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_TerrainHeightfieldCollision, ezSceneExportModifier);

public:
  virtual void ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport) override;
};
