#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

/// Script extension class providing access to console variables (CVars) from scripts.
///
/// Allows scripts to read and modify CVars for configuration and debugging purposes.
/// Provides type-safe accessors for common CVar types as well as generic variant access.
class EZ_CORE_DLL ezScriptExtensionClass_CVar
{
public:
  static ezVariant GetValue(ezStringView sName);
  static bool GetBoolValue(ezStringView sName);
  static int GetIntValue(ezStringView sName);
  static float GetFloatValue(ezStringView sName);
  static ezString GetStringValue(ezStringView sName);

  static void SetValue(ezStringView sName, const ezVariant& value);
  static void SetBoolValue(ezStringView sName, bool bValue);
  static void SetIntValue(ezStringView sName, int iValue);
  static void SetFloatValue(ezStringView sName, float fValue);
  static void SetStringValue(ezStringView sName, const ezString& sValue);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptExtensionClass_CVar);
