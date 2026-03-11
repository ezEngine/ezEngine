#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Settings/ToolsTagRegistry.h>

ezStatus ezQtEditorApp::SaveTagRegistry()
{
  EZ_LOG_BLOCK("ezQtEditorApp::SaveTagRegistry()");

  ezStringBuilder sPath;
  sPath = ezToolsProject::GetSingleton()->GetProjectDirectory();
  sPath.AppendPath("RuntimeConfigs/Tags.ddl");

  ezDeferredFileWriter file;
  file.SetOutput(sPath);

  ezToolsTagRegistry::WriteToDDL(file);

  if (file.Close().Failed())
  {
    return ezStatus(ezFmt("Could not open tags config file '{0}' for writing", sPath));
  }

  return ezStatus(EZ_SUCCESS);
}

void ezQtEditorApp::ReadTagRegistry()
{
  EZ_LOG_BLOCK("ezQtEditorApp::ReadTagRegistry");

  ezToolsTagRegistry::Clear();

  ezStringBuilder sPath;
  sPath = ezToolsProject::GetSingleton()->GetProjectDirectory();
  sPath.AppendPath("RuntimeConfigs/Tags.ddl");

  ezFileReader file;
  if (file.Open(sPath).Failed())
  {
    ezLog::Warning("Could not open tags config file '{0}'", sPath);

    SaveTagRegistry().LogFailure();
  }
  else
  {
    ezToolsTagRegistry::ReadFromDDL(file).LogFailure();
  }


  // TODO: Add default tags
  ezToolsTag tag;
  tag.m_sName = "EditorHidden";
  tag.m_sCategory = "Editor";
  ezToolsTagRegistry::AddTag(tag);
}
