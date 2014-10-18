#include <PCH.h>
#include <EditorFramework/EditorFramework.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Configuration/Plugin.h>

ezPluginSet ezEditorFramework::s_EditorPluginsAvailable;
ezPluginSet ezEditorFramework::s_EditorPluginsActive;
ezPluginSet ezEditorFramework::s_EditorPluginsToBeLoaded;

const ezPluginSet& ezEditorFramework::GetEditorPluginsAvailable()
{
#if EZ_ENABLED(EZ_SUPPORTS_FILE_ITERATORS)
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
#endif

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
