#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>

#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

class EZ_ENGINEPLUGINSCENE_DLL ezSceneExportModifier_RemoveShapeIconComponents : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_RemoveShapeIconComponents, ezSceneExportModifier);

public:
  virtual void ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport) override;
};

//////////////////////////////////////////////////////////////////////////

class EZ_ENGINEPLUGINSCENE_DLL ezSceneExportModifier_RemovePathNodeComponents : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_RemovePathNodeComponents, ezSceneExportModifier);

public:
  virtual void ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport) override;
};

//////////////////////////////////////////////////////////////////////////

class EZ_ENGINEPLUGINSCENE_DLL ezSceneExportModifier_GenericExport : public ezSceneExportModifier
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier_GenericExport, ezSceneExportModifier);

public:
  virtual void ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport) override;
};
