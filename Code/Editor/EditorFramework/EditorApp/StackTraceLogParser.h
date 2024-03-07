#pragma once

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorFrameworkDLL.h>

namespace ezStackTraceLogParser
{
  bool EZ_EDITORFRAMEWORK_DLL ParseStackTraceFileNameAndLineNumber(const ezStringView& sLine, ezStringView& ref_sFileName, ezInt32& ref_iLineNumber); // [tested]
  void Register();
  void Unregister();
} // namespace ezStackTraceLogParser