#include <PCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

void ezQtEditorApp::SaveRecentFiles()
{
  s_RecentProjects.Save(":appdata/Settings/RecentProjects.txt");
  s_RecentDocuments.Save(":appdata/Settings/RecentDocuments.txt");
}

void ezQtEditorApp::LoadRecentFiles()
{
  s_RecentProjects.Load(":appdata/Settings/RecentProjects.txt");
  s_RecentDocuments.Load(":appdata/Settings/RecentDocuments.txt");
}

void ezQtEditorApp::SaveOpenDocumentsList()
{
  const ezDynamicArray<ezQtDocumentWindow*>& windows = ezQtDocumentWindow::GetAllDocumentWindows();

  if (windows.IsEmpty())
    return;

  ezRecentFilesList allDocs(windows.GetCount());

  ezDynamicArray<ezQtDocumentWindow*> allWindows;
  allWindows.Reserve(windows.GetCount());
  const auto& containers = ezQtContainerWindow::GetAllContainerWindows();
  for (auto* container : containers)
  {
    ezHybridArray<ezQtDocumentWindow*, 16> windows;
    container->GetDocumentWindows(windows);
    for (auto* pWindow : windows)
    {
      allWindows.PushBack(pWindow);
    }
  }

  for (ezInt32 w = (ezInt32)allWindows.GetCount() - 1; w >= 0; --w)
  {
    if (allWindows[w]->GetDocument())
    {
      allDocs.Insert(allWindows[w]->GetDocument()->GetDocumentPath(), allWindows[w]->GetContainerWindow()->GetUniqueIdentifier());
    }
  }

  ezStringBuilder sFile = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  sFile.AppendPath("LastDocuments.txt");

  allDocs.Save(sFile);
}

ezRecentFilesList ezQtEditorApp::LoadOpenDocumentsList()
{
  ezRecentFilesList allDocs(15);

  ezStringBuilder sFile = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
  sFile.AppendPath("LastDocuments.txt");

  allDocs.Load(sFile);

  return allDocs;
}

void ezQtEditorApp::SaveSettings()
{
  SaveRecentFiles();

  ezPreferences::SaveApplicationPreferences();

  if (ezToolsProject::IsProjectOpen())
  {
    ezPreferences::SaveProjectPreferences();
    SaveOpenDocumentsList();

    m_FileSystemConfig.Save();
    m_EnginePluginConfig.Save();
  }
}
