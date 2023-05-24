#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class ezBlackboard;

class EZ_CORE_DLL ezScriptExtensionClass_Blackboard
{
public:
  static ezBlackboard* GetOrCreateGlobal(ezStringView sName);
  static ezBlackboard* FindGlobal(ezStringView sName);
  static void RegisterEntry(ezBlackboard* pBoard, ezStringView sName, const ezVariant& initialValue, bool bSave, bool bOnChangeEvent);
  static bool SetEntryValue(ezBlackboard* pBoard, ezStringView sName, const ezVariant& value);
  static ezVariant GetEntryValue(ezBlackboard* pBoard, ezStringView sName, const ezVariant& fallback);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptExtensionClass_Blackboard);
