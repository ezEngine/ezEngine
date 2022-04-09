#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class EZ_CORE_DLL ezScriptExtensionClass_Log
{
public:
  static void Info(const char* szText, const ezVariantArray& params);
  static void Warning(const char* szText, const ezVariantArray& params);
  static void Error(const char* szText, const ezVariantArray& params);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptExtensionClass_Log);
