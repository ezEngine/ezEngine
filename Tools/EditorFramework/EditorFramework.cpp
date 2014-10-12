#include <PCH.h>
#include <EditorFramework/EditorFramework.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/JSONReader.h>
#include <QMainWindow>
#include <QSettings>

ezString ezEditorFramework::s_sApplicationName("ezEditor");
ezString ezEditorFramework::s_sWindowTitle;
bool ezEditorFramework::s_bContentModified = false;


QMainWindow* ezEditorFramework::s_pMainWindow = nullptr;

ezPluginSet ezEditorFramework::s_EditorPluginsAvailable;
ezPluginSet ezEditorFramework::s_EditorPluginsActive;
ezPluginSet ezEditorFramework::s_EditorPluginsToBeLoaded;

ezSet<ezString> ezEditorFramework::s_RestartRequiredReasons;


const ezPluginSet& ezEditorFramework::GetEditorPluginsAvailable()
{
  if (s_EditorPluginsAvailable.m_Plugins.IsEmpty())
  {
    ezStringBuilder sSearch = ezOSFile::GetApplicationDirectory();;
    sSearch.AppendPath("ezEditorPlugin*.dll");

    ezFileSystemIterator fsit;
    if (fsit.StartSearch(sSearch.GetData(), false, false).Succeeded())
    {
      do
      {
        ezStringBuilder sPlugin = fsit.GetStats().m_sFileName;
        sPlugin.Shrink(0, 4); // TODO: ChangeFileExtension should work with empty extensions...

        s_EditorPluginsAvailable.m_Plugins.Insert(sPlugin);
      }
      while(fsit.Next().Succeeded());
    }
  }

  return s_EditorPluginsAvailable;
}

void ezEditorFramework::SetEditorPluginsToBeLoaded(const ezPluginSet& plugins)
{
  s_EditorPluginsToBeLoaded = plugins;
  AddRestartRequiredReason("The set of active plugins has changed.");

  ezFileWriter FileOut;
  FileOut.Open("ActivePlugins.json");

  ezStandardJSONWriter writer;
  writer.SetOutputStream(&FileOut);

  writer.BeginObject();
    writer.BeginArray("Plugins");

    for (auto it = plugins.m_Plugins.GetIterator(); it.IsValid(); ++it)
    {
      writer.WriteString(it.Key().GetData());
    }

    writer.EndArray();
  writer.EndObject();
}

void ezEditorFramework::ReadPluginsToBeLoaded()
{
  s_EditorPluginsToBeLoaded.m_Plugins.Clear();

  ezFileReader FileIn;
  if (FileIn.Open("ActivePlugins.json").Failed())
    return;

  ezJSONReader reader;
  if (reader.Parse(FileIn).Failed())
    return;

  ezVariant* pValue;
  if (!reader.GetTopLevelObject().TryGetValue("Plugins", pValue) || !pValue->IsA<ezVariantArray>())
    return;

  ezVariantArray plugins = pValue->ConvertTo<ezVariantArray>();

  for (ezUInt32 i = 0; i < plugins.GetCount(); ++i)
  {
    const ezString sPlugin = plugins[i].ConvertTo<ezString>();

    s_EditorPluginsToBeLoaded.m_Plugins.Insert(sPlugin);
  }
}


void ezEditorFramework::LoadPlugins()
{
  EZ_ASSERT(s_EditorPluginsActive.m_Plugins.IsEmpty(), "Plugins were already loaded.");

  GetEditorPluginsAvailable();
  ReadPluginsToBeLoaded();

  for (auto it = s_EditorPluginsToBeLoaded.m_Plugins.GetIterator(); it.IsValid(); ++it)
  {
    // only load plugins that are available
    if (s_EditorPluginsAvailable.m_Plugins.Find(it.Key().GetData()).IsValid())
    {
      ezPlugin::LoadPlugin(it.Key().GetData());
    }
  }

  s_EditorPluginsActive = s_EditorPluginsToBeLoaded;
}

void ezEditorFramework::SetEditorWindowTitle(const char* szTitle)
{
  if (s_sWindowTitle == szTitle)
    return;

  s_sWindowTitle = szTitle;

  UpdateEditorWindowTitle();
}

void ezEditorFramework::UpdateEditorWindowTitle()
{
  ezStringBuilder sTitle = s_sApplicationName;

  if (!s_sWindowTitle.IsEmpty())
  {
    sTitle.Append(" - ", s_sWindowTitle.GetData());
  }

  if (s_bContentModified)
  {
    sTitle.Append("*");
  }

  if (s_pMainWindow)
    s_pMainWindow->setWindowTitle(QString::fromUtf8(sTitle.GetData()));
}

void ezEditorFramework::SaveWindowLayout()
{
  if (!s_pMainWindow)
    return;

  const bool bMaximized = s_pMainWindow->isMaximized();

  if (bMaximized)
    s_pMainWindow->showNormal();

  QSettings Settings;
  Settings.beginGroup("EditorMainWnd");
  {
    Settings.setValue("WindowGeometry", s_pMainWindow->saveGeometry());
    Settings.setValue("WindowState", s_pMainWindow->saveState());
    Settings.setValue("IsMaximized", bMaximized);
    Settings.setValue("WindowPosition", s_pMainWindow->pos());

    if (!bMaximized)
      Settings.setValue("WindowSize", s_pMainWindow->size());
  }
  Settings.endGroup();
}

void ezEditorFramework::RestoreWindowLayout()
{
  if (!s_pMainWindow)
    return;

  QSettings Settings;
  Settings.beginGroup("EditorMainWnd");
  {
    s_pMainWindow->restoreGeometry(Settings.value("WindowGeometry", s_pMainWindow->saveGeometry()).toByteArray());

    s_pMainWindow->move(Settings.value("WindowPosition", s_pMainWindow->pos()).toPoint());
    s_pMainWindow->resize(Settings.value("WindowSize", s_pMainWindow->size()).toSize());

    if (Settings.value("IsMaximized", s_pMainWindow->isMaximized()).toBool())
      s_pMainWindow->showMaximized();

    s_pMainWindow->restoreState(Settings.value("WindowState", s_pMainWindow->saveState()).toByteArray());
  }
  Settings.endGroup();
}