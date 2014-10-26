#include <PCH.h>
#include <EditorFramework/EditorFramework.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

ezString ezEditorFramework::s_sProjectPath;

ezString ezEditorFramework::GetProjectDataFolder()
{
  ezStringBuilder sPath = GetProjectPath();
  sPath.Append("_data");
  return sPath;
}

ezResult ezEditorFramework::CreateProject(ezStringView sProjectPath)
{
  EZ_ASSERT(!sProjectPath.IsEmpty(), "Path cannot be empty.");

  CloseProject();

  {
    ezFileWriter ProjectFile;
    if (ProjectFile.Open(sProjectPath.GetData()).Failed())
    {
      ezLog::Error("Could not open/create project file '%s'", sProjectPath.GetData());
      return EZ_FAILURE;
    }
  }

  // Create default folders
  {
    ezStringBuilder sPath;

    sPath = sProjectPath;
    sPath.PathParentDirectory();
    sPath.AppendPath("Scenes");

    ezOSFile::CreateDirectoryStructure(sPath.GetData());
  }

  EditorEvent e;
  e.m_Type = EditorEvent::EventType::OnCreateProject;
  e.m_sPath = sProjectPath;
  s_EditorEvents.Broadcast(e);

  return OpenProject(sProjectPath);
}

ezResult ezEditorFramework::OpenProject(ezStringView sProjectPath)
{
  EZ_ASSERT(!sProjectPath.IsEmpty(), "Path cannot be empty.");

  // do not open the same project twice
  if (s_sProjectPath == sProjectPath)
    return EZ_SUCCESS;

  CloseProject();

  s_sProjectPath = sProjectPath;
  UpdateEditorWindowTitle();

  // load the settings
  GetSettings(SettingsCategory::Project);

  EditorEvent e;
  e.m_Type = EditorEvent::EventType::BeforeOpenProject;
  s_EditorEvents.Broadcast(e);

  if (LoadProject().Failed())
  {
    CloseProject();
    return EZ_FAILURE;
  }

  e.m_Type = EditorEvent::EventType::AfterOpenProject;
  s_EditorEvents.Broadcast(e);

  SaveSettings();
  return EZ_SUCCESS;
}

void ezEditorFramework::CloseProject()
{
  if (s_sProjectPath.IsEmpty())
    return;

  //CloseScene();

  EditorEvent e;
  e.m_Type = EditorEvent::EventType::BeforeCloseProject;
  s_EditorEvents.Broadcast(e);

  UnloadProject();

  e.m_Type = EditorEvent::EventType::AfterCloseProject;
  s_EditorEvents.Broadcast(e);

  ClearSettingsProject();
  s_sProjectPath.Clear();

  UpdateEditorWindowTitle();
}

ezResult ezEditorFramework::LoadProject()
{
  ezFileReader reader;
  if (reader.Open(s_sProjectPath.GetData()).Failed())
    return EZ_FAILURE;

  // TODO

  return EZ_SUCCESS;
}

void ezEditorFramework::UnloadProject()
{
}