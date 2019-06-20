#pragma once

#include <EnginePluginPhysX/EnginePluginPhysXDLL.h>
#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

class EZ_ENGINEPLUGINPHYSX_DLL ezSceneExportModifier_StaticMeshConversion : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_StaticMeshConversion, ezSceneExportModifier);
public:

  virtual void ModifyWorld(ezWorld& world, const ezUuid& documentGuid) override;
};

