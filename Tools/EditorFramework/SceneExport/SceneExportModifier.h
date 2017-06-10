#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Reflection/Reflection.h>

class ezWorld;

class EZ_EDITORFRAMEWORK_DLL ezSceneExportModifier : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneExportModifier, ezReflectedClass);

public:

  static void CreateModifiers(ezHybridArray<ezSceneExportModifier*, 8>& modifiers);
  static void DestroyModifiers(ezHybridArray<ezSceneExportModifier*, 8>& modifiers);

  static void ApplyAllModifiers(ezWorld& world);

  virtual void ModifyWorld(ezWorld& world) = 0;

};


