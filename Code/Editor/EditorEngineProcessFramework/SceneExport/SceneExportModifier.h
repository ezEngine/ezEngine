#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class ezWorld;

class EZ_EDITORENGINEPROCESSFRAMEWORK_DLL ezSceneExportModifier : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier, ezReflectedClass);

public:

  static void CreateModifiers(ezHybridArray<ezSceneExportModifier*, 8>& modifiers);
  static void DestroyModifiers(ezHybridArray<ezSceneExportModifier*, 8>& modifiers);

  static void ApplyAllModifiers(ezWorld& world, const ezUuid& documentGuid);

  virtual void ModifyWorld(ezWorld& world, const ezUuid& documentGuid) = 0;

};


