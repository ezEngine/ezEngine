#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppSettings.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

ezResult ezCppSettings::Save(ezStringView sFile)
{
  ezFileWriter file;
  EZ_SUCCEED_OR_RETURN(file.Open(sFile));

  ezOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  ddl.BeginObject("Target", "Default");

  ezOpenDdlUtils::StoreString(ddl, m_sPluginName, "PluginName");

  ddl.EndObject();

  return EZ_SUCCESS;
}

ezResult ezCppSettings::Load(ezStringView sFile)
{
  ezFileReader file;
  EZ_SUCCEED_OR_RETURN(file.Open(sFile));

  ezOpenDdlReader ddl;
  EZ_SUCCEED_OR_RETURN(ddl.ParseDocument(file));

  if (auto pTarget = ddl.GetRootElement()->FindChildOfType("Target", "Default"))
  {
    if (auto pValue = pTarget->FindChildOfType(ezOpenDdlPrimitiveType::String, "PluginName"))
    {
      m_sPluginName = pValue->GetPrimitivesString()[0];
    }
  }

  return EZ_SUCCESS;
}
