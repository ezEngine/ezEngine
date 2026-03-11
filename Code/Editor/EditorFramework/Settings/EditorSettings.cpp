#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  include <EditorFramework/EditorApp/WindowsJumpList.h>
#endif

void ezQtEditorApp::SaveRecentFiles()
{
  EZ_PROFILE_SCOPE("SaveRecentFiles");
  if (m_StartupFlags.IsAnySet(StartupFlags::Headless | StartupFlags::UnitTest | StartupFlags::Background))
    return;

  m_RecentProjects.Save(":appdata/Settings/RecentProjects.txt");
  m_RecentDocuments.Save(":appdata/Settings/RecentDocuments.txt");

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  // Update Windows taskbar jump list with recent projects
  ezWindowsJumpList::UpdateJumpList(m_RecentProjects);
#endif
}

void ezQtEditorApp::LoadRecentFiles()
{
  EZ_PROFILE_SCOPE("LoadRecentFiles");
  m_RecentProjects.Load(":appdata/Settings/RecentProjects.txt");
  m_RecentDocuments.Load(":appdata/Settings/RecentDocuments.txt");
}

void ezQtEditorApp::SaveOpenDocumentsList()
{
  const ezDynamicArray<ezQtDocumentWindow*>& windows = ezQtDocumentWindow::GetAllDocumentWindows();

  if (windows.IsEmpty())
    return;

  ezRecentFilesList allDocs(windows.GetCount());

  ezDynamicArray<ezQtDocumentWindow*> allWindows;
  allWindows.Reserve(windows.GetCount());
  {
    auto* container = ezQtContainerWindow::GetContainerWindow();
    ezTempHybridArray<ezQtDocumentWindow*, 16> docWindows;
    container->GetDocumentWindows(docWindows);
    for (auto* pWindow : docWindows)
    {
      allWindows.PushBack(pWindow);
    }
  }
  for (ezInt32 w = (ezInt32)allWindows.GetCount() - 1; w >= 0; --w)
  {
    if (allWindows[w]->GetDocument())
    {
      allDocs.Insert(allWindows[w]->GetDocument()->GetDocumentPath(), 0);
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
  // headless mode should never store any settings on disk
  if (m_StartupFlags.IsAnySet(StartupFlags::Headless | StartupFlags::UnitTest | StartupFlags::Background))
    return;

  SaveRecentFiles();

  ezPreferences::SaveApplicationPreferences();

  // this setting is needed before we have loaded the preferences, so we duplicate it in the QSettings (registry)
  {
    ezEditorPreferencesUser* pPreferences = ezPreferences::QueryPreferences<ezEditorPreferencesUser>();

    QSettings s;
    s.beginGroup("EditorPreferences");
    s.setValue("ShowSplashscreen", pPreferences->m_bShowSplashscreen);
    s.endGroup();
  }

  if (ezToolsProject::IsProjectOpen())
  {
    ezPreferences::SaveProjectPreferences();
    SaveOpenDocumentsList();

    m_FileSystemConfig.Save().IgnoreResult();
    GetRuntimePluginConfig(false).Save().IgnoreResult();
  }
}
