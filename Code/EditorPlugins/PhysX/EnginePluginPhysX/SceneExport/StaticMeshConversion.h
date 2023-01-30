#pragma once

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>
#include <EnginePluginPhysX/EnginePluginPhysXDLL.h>

class EZ_ENGINEPLUGINPHYSX_DLL ezSceneExportModifier_StaticMeshConversion : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_StaticMeshConversion, ezSceneExportModifier);

public:
  virtual void ModifyWorld(ezWorld& world, const ezUuid& documentGuid, bool bForExport) override;
};
