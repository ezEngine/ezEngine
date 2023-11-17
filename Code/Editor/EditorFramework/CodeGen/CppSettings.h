#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Strings/String.h>

class EZ_EDITORFRAMEWORK_DLL ezCppSettings
{
public:
  ezResult Save(ezStringView sFile = ":project/Editor/CppProject.ddl");
  ezResult Load(ezStringView sFile = ":project/Editor/CppProject.ddl");

  ezString m_sPluginName;
};
