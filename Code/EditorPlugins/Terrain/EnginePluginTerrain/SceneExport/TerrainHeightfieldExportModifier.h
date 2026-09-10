#pragma once

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginTerrain/EnginePluginTerrainDLL.h>

/// Scene export modifier that bakes terrain heightfield colliders and occluders from the GPU height data.
///
/// For each ezTerrainPatchComponent with a non-None collider mode, it reads back the GPU-baked height data,
/// cooks a Jolt heightfield shape, and writes a .ezBinJoltHeightfield file to AssetCache/Generated/. A child
/// game object named "HeightfieldCollider" is then created (or reused) with an
/// ezJoltHeightfieldColliderComponent pointing to that file.
///
/// For each patch with a non-zero occlusion cell size, it additionally computes a coarse, conservative mesh and
/// stores it on the component, from where the patch builds its CPU occlusion culling geometry. Both use the
/// same readback, which is why they live in one modifier: a readback forces a full re-bake of the patch.
class EZ_ENGINEPLUGINTERRAIN_DLL ezSceneExportModifier_TerrainHeightfieldCollision : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_TerrainHeightfieldCollision, ezSceneExportModifier);

public:
  virtual void ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport) override;
};
