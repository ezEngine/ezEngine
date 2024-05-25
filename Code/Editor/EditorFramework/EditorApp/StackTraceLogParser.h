#pragma once

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorFrameworkDLL.h>

namespace ezStackTraceLogParser
{
  EZ_EDITORFRAMEWORK_DLL bool ParseStackTraceFileNameAndLineNumber(const ezStringView& sLine, ezStringView& ref_sFileName, ezInt32& ref_iLineNumber); // [tested]
  EZ_EDITORFRAMEWORK_DLL bool ParseAssertFileNameAndLineNumber(const ezStringView& sLine, ezStringView& ref_sFileName, ezInt32& ref_iLineNumber);     // [tested]
  void Register();
  void Unregister();
} // namespace ezStackTraceLogParser