#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Strings/String.h>

class EZ_EDITORFRAMEWORK_DLL ezCppSettings
{
public:
  ezResult Save(ezStringView sFile = ":project/Editor/CppProject.ddl");
  ezResult Load(ezStringView sFile = ":project/Editor/CppProject.ddl");

  enum class Compiler
  {
    None,
    Vs2022,
  };

  ezString m_sPluginName;
  Compiler m_Compiler = Compiler::None;
  mutable ezString m_sMsBuildPath;
};
