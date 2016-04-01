#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>
#include <Foundation/IO/FileSystem/FileReader.h>

ezStatus ezQtEditorApp::SaveTagRegistry()
{
  EZ_LOG_BLOCK("ezQtEditorApp::SaveTagRegistry()");

  ezStringBuilder sPath;
  sPath = ezApplicationConfig::GetProjectDirectory();
  sPath.AppendPath("Tags.ezManifest");

  ezFileWriter file;
  if (file.Open(sPath).Failed())
  {
    return ezStatus("Could not open tags config file '%s' for writing", sPath.GetData());
  }

  ezToolsTagRegistry::WriteToJSON(file);
  return ezStatus(EZ_SUCCESS);
}

void ezQtEditorApp::ReadTagRegistry()
{
  EZ_LOG_BLOCK("ezQtEditorApp::ReadTagRegistry()");

  ezToolsTagRegistry::Clear();

  ezStringBuilder sPath;
  sPath = ezApplicationConfig::GetProjectDirectory();
  sPath.AppendPath("Tags.ezManifest");

  ezFileReader file;
  if (file.Open(sPath).Failed())
  {
    ezLog::Warning("Could not open tags config file '%s'", sPath.GetData());

    ezStatus res = SaveTagRegistry();
    if (res.m_Result.Failed())
    {
      ezLog::Error("%s", res.m_sMessage.GetData());
    }
  }
  else
  {
    ezStatus res = ezToolsTagRegistry::ReadFromJSON(file);
    if (res.m_Result.Failed())
    {
      ezLog::Error("%s", res.m_sMessage.GetData());
    }
  }


  // TODO: Add default tags
  ezToolsTag tag;
  tag.m_sName = "EditorHidden";
  tag.m_sCategory = "Editor";
  ezToolsTagRegistry::AddTag(tag);
}

