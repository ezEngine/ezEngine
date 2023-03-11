#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

class EZ_ENGINEPLUGINSCENE_DLL ezSceneExportModifier_RemoveShapeIconComponents : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_RemoveShapeIconComponents, ezSceneExportModifier);

public:
  virtual void ModifyWorld(ezWorld& ref_world, const ezUuid& documentGuid, bool bForExport) override;
};

//////////////////////////////////////////////////////////////////////////

class EZ_ENGINEPLUGINSCENE_DLL ezSceneExportModifier_RemovePathNodeComponents : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_RemovePathNodeComponents, ezSceneExportModifier);

public:
  virtual void ModifyWorld(ezWorld& world, const ezUuid& documentGuid, bool bForExport) override;
};
