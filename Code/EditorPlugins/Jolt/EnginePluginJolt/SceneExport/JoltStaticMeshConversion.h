#pragma once

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginJolt/EnginePluginJoltDLL.h>

class EZ_ENGINEPLUGINJOLT_DLL ezSceneExportModifier_JoltStaticMeshConversion : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_JoltStaticMeshConversion, ezSceneExportModifier);

public:
  virtual void ModifyWorld(ezWorld& ref_world, const ezUuid& documentGuid, bool bForExport) override;
};
