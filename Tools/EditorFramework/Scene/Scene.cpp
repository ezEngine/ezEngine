#include <PCH.h>
#include <EditorFramework/EditorFramework.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Logging/Log.h>

//ezString ezEditorFramework::GetSceneDataFolder()
//{
//  ezStringBuilder sPath = GetScenePath();
//  sPath.Append("_data");
//  return sPath;
//}
//
//ezResult ezEditorFramework::CreateScene(ezStringView sScenePath)
//{
//  EZ_ASSERT(!sScenePath.IsEmpty(), "Path cannot be empty.");
//
//  CloseScene();
//
//  {
//    ezFileWriter SceneFile;
//    if (SceneFile.Open(sScenePath.GetData()).Failed())
//    {
//      ezLog::Error("Could not open/create scene file '%s'", sScenePath.GetData());
//      return EZ_FAILURE;
//    }
//  }
//
//  // do stuff
//
//  EditorEvent e;
//  e.m_Type = EditorEvent::EventType::OnCreateScene;
//  e.m_sPath = sScenePath;
//  s_EditorEvents.Broadcast(e);
//
//  return OpenScene(sScenePath);
//}
//
//ezResult ezEditorFramework::OpenScene(ezStringView sScenePath)
//{
//  EZ_ASSERT(!sScenePath.IsEmpty(), "Path cannot be empty.");
//
//  CloseScene();
//
//  s_sScenePath = sScenePath;
//  UpdateEditorWindowTitle();
//
//  // load the settings
//  GetSettings(SettingsCategory::Scene);
//
//  EditorEvent e;
//  e.m_Type = EditorEvent::EventType::BeforeOpenScene;
//  s_EditorEvents.Broadcast(e);
//
//  if (LoadScene().Failed())
//  {
//    CloseScene();
//    return EZ_FAILURE;
//  }
//
//  e.m_Type = EditorEvent::EventType::AfterOpenScene;
//  s_EditorEvents.Broadcast(e);
//
//  SaveSettings();
//  return EZ_SUCCESS;
//}
//
//void ezEditorFramework::CloseScene()
//{
//  if (s_sScenePath.IsEmpty())
//    return;
//
//  EditorEvent e;
//  e.m_Type = EditorEvent::EventType::BeforeCloseScene;
//  s_EditorEvents.Broadcast(e);
//
//  UnloadScene();
//
//  e.m_Type = EditorEvent::EventType::AfterCloseScene;
//  s_EditorEvents.Broadcast(e);
//
//  ClearSettingsScene();
//  s_sScenePath.Clear();
//
//  UpdateEditorWindowTitle();
//}
//
//ezResult ezEditorFramework::LoadScene()
//{
//  ezFileReader reader;
//  if (reader.Open(s_sScenePath.GetData()).Failed())
//    return EZ_FAILURE;
//
//  // TODO
//
//  return EZ_SUCCESS;
//}
//
//void ezEditorFramework::UnloadScene()
//{
//}

