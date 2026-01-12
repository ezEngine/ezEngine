#pragma once

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginJolt/EnginePluginJoltDLL.h>

/// \brief A export modifier that finalizes the collision mesh generation from ezJoltGenerateCollisionComponents.
///
/// A static mesh actor which references the generated collision mesh and the generate component is removed from scenes (not prefabs though).
class EZ_ENGINEPLUGINJOLT_DLL ezSceneExportModifier_JoltFinalizeGeneratedCollision : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_JoltFinalizeGeneratedCollision, ezSceneExportModifier);

public:
  virtual void ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport) override;
};
