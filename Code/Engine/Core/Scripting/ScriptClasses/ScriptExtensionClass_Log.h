#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class EZ_CORE_DLL ezScriptExtensionClass_Log
{
public:
  static void Info(ezStringView sText, const ezVariantArray& params);
  static void Warning(ezStringView sText, const ezVariantArray& params);
  static void Error(ezStringView sText, const ezVariantArray& params);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptExtensionClass_Log);
