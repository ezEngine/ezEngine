#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class ezWorld;

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezSceneExportModifier : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier, ezReflectedClass);

public:
  static void CreateModifiers(ezHybridArray<ezSceneExportModifier*, 8>& ref_modifiers);
  static void DestroyModifiers(ezHybridArray<ezSceneExportModifier*, 8>& ref_modifiers);

  static void ApplyAllModifiers(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport);

  virtual void ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport) = 0;

  static void CleanUpWorld(ezWorld& ref_world);
};
