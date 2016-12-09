#include <PCH.h>
#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlReader.h>

void ezQtEditorApp::DetectAvailableEditorPlugins()
{
  s_EditorPlugins.m_Plugins.Clear();

#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)
  {
    ezStringBuilder sSearch = ezOSFile::GetApplicationDirectory();;
    sSearch.AppendPath("ezEditorPlugin*.dll");

    ezFileSystemIterator fsit;
    if (fsit.StartSearch(sSearch.GetData(), false, false).Succeeded())
    {
      do
      {
        ezStringBuilder sPlugin = fsit.GetStats().m_sFileName;
        sPlugin.RemoveFileExtension();

        s_EditorPlugins.m_Plugins[sPlugin].m_bAvailable = true;
      }
      while (fsit.Next().Succeeded());
    }
  }
#endif
}

void ezQtEditorApp::StoreEditorPluginsToBeLoaded()
{
  AddRestartRequiredReason("The set of active editor plugins was changed.");

  ezFileWriter FileOut;
  FileOut.Open(":appdata/EditorPlugins.ddl");

  ezOpenDdlWriter writer;
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(ezOpenDdlWriter::TypeStringMode::Compliant);
  writer.SetOutputStream(&FileOut);

  for (auto it = s_EditorPlugins.m_Plugins.GetIterator(); it.IsValid(); ++it)
  {
    writer.BeginObject("Plugin");
    ezOpenDdlUtils::StoreString(writer, it.Key().GetData(), "Name");
    ezOpenDdlUtils::StoreBool(writer, it.Value().m_bToBeLoaded, "Enable");
    writer.EndObject();
  }
}

void ezQtEditorApp::ReadEditorPluginsToBeLoaded()
{
  // If loading that file fails, enable all plugins
  for (auto it = s_EditorPlugins.m_Plugins.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_bToBeLoaded = true;
  }

  ezFileReader FileIn;
  if (FileIn.Open(":appdata/EditorPlugins.ddl").Failed())
    return;

  ezOpenDdlReader reader;
  if (reader.ParseDocument(FileIn, 0, ezGlobalLog::GetOrCreateInstance()).Failed())
    return;

  const ezOpenDdlReaderElement* pRoot = reader.GetRootElement();

  // if a plugin is new, activate it (by keeping it enabled)

  for (const ezOpenDdlReaderElement* pPlugin = pRoot->GetFirstChild(); pPlugin != nullptr; pPlugin = pPlugin->GetSibling())
  {
    if (!pPlugin->IsCustomType("Plugin"))
      continue;

    const ezOpenDdlReaderElement* pName = pPlugin->FindChildOfType(ezOpenDdlPrimitiveType::String, "Name");
    const ezOpenDdlReaderElement* pEnable = pPlugin->FindChildOfType(ezOpenDdlPrimitiveType::Bool, "Enable");

    if (!pName || !pEnable)
      continue;

    s_EditorPlugins.m_Plugins[pName->GetPrimitivesString()[0]].m_bToBeLoaded = pEnable->GetPrimitivesBool()[0];
  }
}


void ezQtEditorApp::LoadEditorPlugins()
{
  DetectAvailableEditorPlugins();
  ReadEditorPluginsToBeLoaded();

  for (auto it = s_EditorPlugins.m_Plugins.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_bActive = false;
  }

  ezSet<ezString> NotLoaded;

  for (auto it = s_EditorPlugins.m_Plugins.GetIterator(); it.IsValid(); ++it)
  {
    if (!it.Value().m_bToBeLoaded)
      continue;

    // only load plugins that are available
    if (it.Value().m_bAvailable)
    {
      if (ezPlugin::LoadPlugin(it.Key().GetData()).Failed())
      {
        NotLoaded.Insert(it.Key());
      }
      else
      {
        it.Value().m_bActive = true;
      }
    }
    else
    {
      NotLoaded.Insert(it.Key());
    }
  }

  if (!NotLoaded.IsEmpty())
  {
    ezStringBuilder s = "The following plugins could not be loaded. Scenes may not load correctly.\n\n";

    for (auto it = NotLoaded.GetIterator(); it.IsValid(); ++it)
    {
      s.AppendPrintf(" '%s' ", it.Key().GetData());
    }

    ezQtUiServices::MessageBoxWarning(s);
  }
}

void ezQtEditorApp::UnloadEditorPlugins()
{
  for (auto it = s_EditorPlugins.m_Plugins.GetLastIterator(); it.IsValid(); --it)
  {
    if (it.Value().m_bActive)
    {
      ezPlugin::UnloadPlugin(it.Key().GetData());
      it.Value().m_bActive = false;
    }
  }
}

