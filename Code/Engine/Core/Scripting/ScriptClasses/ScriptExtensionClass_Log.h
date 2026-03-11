#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

/// Script extension class providing logging functionality from scripts.
///
/// Allows scripts to output formatted log messages at different severity levels.
/// Messages are sent to the standard ezEngine logging system and will appear
/// in the console, log files, and other registered log writers.
class EZ_CORE_DLL ezScriptExtensionClass_Log
{
public:
  static void Info(ezStringView sText, const ezVariantArray& params);
  static void Warning(ezStringView sText, const ezVariantArray& params);
  static void Error(ezStringView sText, const ezVariantArray& params);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptExtensionClass_Log);
