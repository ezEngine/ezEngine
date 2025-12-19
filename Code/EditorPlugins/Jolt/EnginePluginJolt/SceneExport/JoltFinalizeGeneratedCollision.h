#pragma once

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginJolt/EnginePluginJoltDLL.h>

class EZ_ENGINEPLUGINJOLT_DLL ezSceneExportModifier_JoltFinalizeGeneratedCollision : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_JoltFinalizeGeneratedCollision, ezSceneExportModifier);

public:
  virtual void ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport) override;
};
