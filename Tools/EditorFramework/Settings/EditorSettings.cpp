#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <Foundation/IO/ExtendedJSONReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorFramework/Preferences/Preferences.h>

void ezQtEditorApp::SaveRecentFiles()
{
  s_RecentProjects.Save("Settings/RecentProjects.txt");
  s_RecentDocuments.Save("Settings/RecentDocuments.txt");
}

void ezQtEditorApp::LoadRecentFiles()
{
  s_RecentProjects.Load("Settings/RecentProjects.txt");
  s_RecentDocuments.Load("Settings/RecentDocuments.txt");
}

void ezQtEditorApp::SaveOpenDocumentsList()
{
  const ezDynamicArray<ezQtDocumentWindow*>& windows = ezQtDocumentWindow::GetAllDocumentWindows();

  if (windows.IsEmpty())
    return;

  ezRecentFilesList allDocs(10);

  ezMap<ezUInt32, ezQtDocumentWindow*> Sorted;

  for (ezUInt32 w = 0; w < windows.GetCount(); ++w)
  {
    if (windows[w]->GetDocument())
    {
      Sorted[windows[w]->GetWindowIndex()] = windows[w];
    }
  }

  for (auto it = Sorted.GetLastIterator(); it.IsValid(); --it)
  {
    allDocs.Insert(it.Value()->GetDocument()->GetDocumentPath());
  }

  ezStringBuilder sFile = GetProjectUserDataFolder();
  sFile.AppendPath("LastDocuments.txt");

  allDocs.Save(sFile);
}

ezRecentFilesList ezQtEditorApp::LoadOpenDocumentsList()
{
  ezRecentFilesList allDocs(15);

  ezStringBuilder sFile = GetProjectUserDataFolder();
  sFile.AppendPath("LastDocuments.txt");

  allDocs.Load(sFile);

  return allDocs;
}

void ezQtEditorApp::SaveSettings()
{
  SaveRecentFiles();

  ezPreferences::SaveProjectAndEditorPreferences();

  if (ezToolsProject::IsProjectOpen())
  {
    SaveOpenDocumentsList();

    m_FileSystemConfig.Save();
    m_EnginePluginConfig.Save();
  }
}



