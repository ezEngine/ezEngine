#include <PCH.h>
#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/JSONReader.h>

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
  FileOut.Open("ActivePlugins.json");

  ezStandardJSONWriter writer;
  writer.SetOutputStream(&FileOut);

  writer.BeginObject();
  writer.BeginArray("Plugins");

  for (auto it = s_EditorPlugins.m_Plugins.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_bToBeLoaded)
      writer.WriteString(it.Key().GetData());
  }

  writer.EndArray();
  writer.EndObject();
}

void ezQtEditorApp::ReadEditorPluginsToBeLoaded()
{
  // If loading that file fails, enable all plugins
  for (auto it = s_EditorPlugins.m_Plugins.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_bToBeLoaded = true;
  }

  ezFileReader FileIn;
  if (FileIn.Open("ActivePlugins.json").Failed())
    return;

  ezJSONReader reader;
  if (reader.Parse(FileIn).Failed())
    return;

  ezVariant* pValue;
  if (!reader.GetTopLevelObject().TryGetValue("Plugins", pValue) || !pValue->IsA<ezVariantArray>())
    return;

  for (auto it = s_EditorPlugins.m_Plugins.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_bToBeLoaded = false;
  }

  ezVariantArray plugins = pValue->ConvertTo<ezVariantArray>();

  for (ezUInt32 i = 0; i < plugins.GetCount(); ++i)
  {
    const ezString sPlugin = plugins[i].ConvertTo<ezString>();

    s_EditorPlugins.m_Plugins[sPlugin].m_bToBeLoaded = true;
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
      s.AppendFormat(" '%s' ", it.Key().GetData());
    }

    ezUIServices::MessageBoxWarning(s);
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

