#include <PCH.h>
#include <EditorFramework/Settings/Settings.h>
#include <EditorFramework/EditorApp.moc.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/ExtendedJSONWriter.h>
#include <Foundation/IO/ExtendedJSONReader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>


void ezSettings::RegisterValueBool(const char* szKey, bool Default, ezBitflags<ezSettingsFlags> flags)
{
  if (!m_Settings.Find(szKey).IsValid())
    m_Settings[szKey].m_Value = Default;

  m_Settings[szKey].m_Flags = flags | ezSettingsFlags::Registered;
}

void ezSettings::RegisterValueInt(const char* szKey, ezInt32 Default, ezBitflags<ezSettingsFlags> flags)
{
  if (!m_Settings.Find(szKey).IsValid())
    m_Settings[szKey].m_Value = Default;

  m_Settings[szKey].m_Flags = flags | ezSettingsFlags::Registered;
}

void ezSettings::RegisterValueFloat(const char* szKey, float Default, ezBitflags<ezSettingsFlags> flags)
{
  if (!m_Settings.Find(szKey).IsValid())
    m_Settings[szKey].m_Value = Default;

  m_Settings[szKey].m_Flags = flags | ezSettingsFlags::Registered;
}
void ezSettings::RegisterValueString(const char* szKey, const char* Default, ezBitflags<ezSettingsFlags> flags) 
{
  if (!m_Settings.Find(szKey).IsValid())
    m_Settings[szKey].m_Value = Default;

  m_Settings[szKey].m_Flags = flags | ezSettingsFlags::Registered;
}

void ezSettings::RegisterValueColor(const char* szKey, const ezColor& Default, ezBitflags<ezSettingsFlags> flags)
{
  if (!m_Settings.Find(szKey).IsValid())
    m_Settings[szKey].m_Value = Default;

  m_Settings[szKey].m_Flags = flags | ezSettingsFlags::Registered;
}


void ezSettings::SetValueBool(const char* szKey, bool value)
{
  auto it = m_Settings.Find(szKey);
  EZ_ASSERT_DEV(it.IsValid() && it.Value().m_Value.IsA<bool>(), "The setting '%s' has not been registered as type 'bool'", szKey);
  m_Settings[szKey].m_Value = value;
}

void ezSettings::SetValueInt(const char* szKey, ezInt32 value)
{
  auto it = m_Settings.Find(szKey);
  EZ_ASSERT_DEV(it.IsValid() && it.Value().m_Value.IsA<ezInt32>(), "The setting '%s' has not been registered as type 'int'", szKey);
  m_Settings[szKey].m_Value = value;
}

void ezSettings::SetValueFloat(const char* szKey, float value)
{
  auto it = m_Settings.Find(szKey);
  EZ_ASSERT_DEV(it.IsValid() && it.Value().m_Value.IsA<float>(), "The setting '%s' has not been registered as type 'float'", szKey);
  m_Settings[szKey].m_Value = value;
}

void ezSettings::SetValueString(const char* szKey, const char* value)
{
  auto it = m_Settings.Find(szKey);
  EZ_ASSERT_DEV(it.IsValid() && it.Value().m_Value.IsA<ezString>(), "The setting '%s' has not been registered as type 'string'", szKey);
  m_Settings[szKey].m_Value = value;
}

void ezSettings::SetValueColor(const char* szKey, const ezColor& value)
{
  auto it = m_Settings.Find(szKey);
  EZ_ASSERT_DEV(it.IsValid() && it.Value().m_Value.IsA<ezColor>(), "The setting '%s' has not been registered as type 'color'", szKey);
  m_Settings[szKey].m_Value = value;
}


bool ezSettings::GetValueBool(const char* szKey)
{
  auto it = m_Settings.FindOrAdd(szKey);
  EZ_ASSERT_DEV(it.IsValid() && it.Value().m_Flags.IsAnySet(ezSettingsFlags::Registered) && it.Value().m_Value.IsA<bool>(), "The setting '%s' has not been registered as type 'bool'", szKey);

  if (!it.IsValid())
    return false;

  return it.Value().m_Value.ConvertTo<bool>();
}

ezInt32 ezSettings::GetValueInt(const char* szKey)
{
  auto it = m_Settings.FindOrAdd(szKey);
  EZ_ASSERT_DEV(it.IsValid() && it.Value().m_Flags.IsAnySet(ezSettingsFlags::Registered) && it.Value().m_Value.IsA<ezInt32>(), "The setting '%s' has not been registered as type 'int'", szKey);

  if (!it.IsValid())
    return 0;

  return it.Value().m_Value.ConvertTo<ezInt32>();

}

float ezSettings::GetValueFloat(const char* szKey)
{
  auto it = m_Settings.FindOrAdd(szKey);
  EZ_ASSERT_DEV(it.IsValid() && it.Value().m_Flags.IsAnySet(ezSettingsFlags::Registered) && it.Value().m_Value.IsA<float>(), "The setting '%s' has not been registered as type 'float'", szKey);

  if (!it.IsValid())
    return 0.0f;

  return it.Value().m_Value.ConvertTo<float>();

}

ezString ezSettings::GetValueString(const char* szKey)
{
  auto it = m_Settings.FindOrAdd(szKey);
  EZ_ASSERT_DEV(it.IsValid() && it.Value().m_Flags.IsAnySet(ezSettingsFlags::Registered) && it.Value().m_Value.IsA<ezString>(), "The setting '%s' has not been registered as type 'string'", szKey);

  if (!it.IsValid())
    return "";

  return it.Value().m_Value.ConvertTo<ezString>();

}

ezColor ezSettings::GetValueColor(const char* szKey)
{
  auto it = m_Settings.FindOrAdd(szKey);
  EZ_ASSERT_DEV(it.IsValid() && it.Value().m_Flags.IsAnySet(ezSettingsFlags::Registered) && it.Value().m_Value.IsA<ezColor>(), "The setting '%s' has not been registered as type 'color'", szKey);

  if (!it.IsValid())
    return ezColor(0,0,0);

  return it.Value().m_Value.ConvertTo<ezColor>();

}

void ezSettings::WriteToJSON(ezStreamWriterBase& stream, bool bNonUserSettings, bool bUserSettings) const
{
  ezExtendedJSONWriter writer;
  writer.SetOutputStream(&stream);
  
  writer.BeginObject();

  for (auto it = m_Settings.GetIterator(); it.IsValid(); ++it)
  {
    if (!it.Value().m_Flags.IsAnySet(ezSettingsFlags::Registered))
      continue;

    const bool bIsUserSetting = it.Value().m_Flags.IsAnySet(ezSettingsFlags::User);

    if (bIsUserSetting && !bUserSettings || !bIsUserSetting && !bNonUserSettings)
      continue;

    const char* szKey = it.Key().GetData();

    switch (it.Value().m_Value.GetType())
    {
    case ezVariant::Type::Bool:
      writer.AddVariableBool(szKey, it.Value().m_Value.ConvertTo<bool>());
      break;
    case ezVariant::Type::Int32:
      writer.AddVariableInt32(szKey, it.Value().m_Value.ConvertTo<ezInt32>());
      break;
    case ezVariant::Type::Float:
      writer.AddVariableFloat(szKey, it.Value().m_Value.ConvertTo<float>());
      break;
    case ezVariant::Type::String:
      writer.AddVariableString(szKey, it.Value().m_Value.ConvertTo<ezString>().GetData());
      break;
    case ezVariant::Type::Color:
      writer.AddVariableColor(szKey, it.Value().m_Value.ConvertTo<ezColor>());
      break;
    }
  }

  writer.EndObject();
}

void ezSettings::ReadFromJSON(ezStreamReaderBase& stream)
{
  ezExtendedJSONReader reader;
  if (reader.Parse(stream).Failed())
    return;

  const ezVariantDictionary& dict = reader.GetTopLevelObject();

  for (auto it = dict.GetIterator(); it.IsValid(); ++it)
  {
    switch (it.Value().GetType())
    {
    case ezVariant::Type::Bool:
    case ezVariant::Type::Int32:
    case ezVariant::Type::Float:
    case ezVariant::Type::String:
    case ezVariant::Type::Color:
      break;

    default:
      continue;
    }

    m_Settings[it.Key()].m_Value = it.Value();
  }
}

void ezEditorApp::RegisterPluginNameForSettings(const char* szPluginName)
{
  s_SettingsPluginNames.Insert(szPluginName);
}

ezSettings& ezEditorApp::GetEditorSettings(const char* szPlugin)
{
  return GetSettings(s_EditorSettings, szPlugin, "");
}

ezSettings& ezEditorApp::GetProjectSettings(const char* szPlugin)
{
  EZ_ASSERT_DEV(ezEditorProject::IsProjectOpen(), "No project is open");

  return GetSettings(s_ProjectSettings, szPlugin, ezEditorApp::GetDocumentDataFolder(ezEditorProject::GetInstance()->GetProjectPath()));
}

ezSettings& ezEditorApp::GetDocumentSettings(const ezDocumentBase* pDocument, const char* szPlugin)
{
  return GetDocumentSettings(pDocument->GetDocumentPath(), szPlugin);
}

ezSettings& ezEditorApp::GetDocumentSettings(const char* szDocument, const char* szPlugin)
{
  return GetSettings(s_DocumentSettings[szDocument], szPlugin, ezEditorApp::GetDocumentDataFolder(szDocument));
}

ezSettings& ezEditorApp::GetSettings(ezMap<ezString, ezSettings>& SettingsMap, const char* szPlugin, const char* szSearchPath)
{
  EZ_ASSERT_DEV(s_SettingsPluginNames.Contains(szPlugin), "The plugin name '%s' has not been registered with 'ezEditorApp::RegisterPluginNameForSettings'", szPlugin);

  bool bExisted = false;

  auto itSett = SettingsMap.FindOrAdd(szPlugin, &bExisted);

  ezSettings& settings = itSett.Value();

  if (!bExisted)
  {
    ezStringBuilder sPath = szSearchPath;

    sPath.AppendPath("Settings", szPlugin);
    sPath.ChangeFileExtension("settings");

    ezFileReader file;
    if (file.Open(sPath.GetData()).Succeeded())
    {
      settings.ReadFromJSON(file);
      file.Close();
    }

    ezStringBuilder sUserFile;
    sUserFile.Append(GetApplicationUserName().GetData(), ".usersettings");
    sPath.ChangeFileExtension(sUserFile.GetData());

    if (file.Open(sPath.GetData()).Succeeded())
    {
      settings.ReadFromJSON(file);
      file.Close();
    }
  }

  return settings;
}

void ezEditorApp::SaveRecentFiles()
{
  s_RecentProjects.Save("Settings/RecentProjects.txt");
  s_RecentDocuments.Save("Settings/RecentDocuments.txt");
}

void ezEditorApp::LoadRecentFiles()
{
  s_RecentProjects.Load("Settings/RecentProjects.txt");
  s_RecentDocuments.Load("Settings/RecentDocuments.txt");
}

void ezEditorApp::StoreSettings(const ezMap<ezString, ezSettings>& settings, const char* szFolder)
{
  for (auto it = settings.GetIterator(); it.IsValid(); ++it)
  {
    const ezSettings& settings = it.Value();

    ezStringBuilder sPath = szFolder;
    sPath.AppendPath("Settings", it.Key().GetData());
    sPath.ChangeFileExtension("settings");

    ezFileWriter file;
    if (file.Open(sPath.GetData()).Succeeded())
    {
      settings.WriteToJSON(file, true, false);
      file.Close();
    }

    ezStringBuilder sUserFile;
    sUserFile.Append(GetApplicationUserName().GetData(), ".usersettings");
    sPath.ChangeFileExtension(sUserFile.GetData());

    if (file.Open(sPath.GetData()).Succeeded())
    {
      settings.WriteToJSON(file, false, true);
      file.Close();
    }
  }
}

void ezEditorApp::SaveSettings()
{
  SaveRecentFiles();

  StoreSettings(s_EditorSettings, "");

  if (ezEditorProject::IsProjectOpen())
  {
    StoreSettings(s_ProjectSettings, GetDocumentDataFolder(ezEditorProject::GetInstance()->GetProjectPath()));
  }
}

void ezEditorApp::SaveDocumentSettings(const ezDocumentBase* pDocument)
{
  auto it = s_DocumentSettings.Find(pDocument->GetDocumentPath());

  if (!it.IsValid())
    return;

  StoreSettings(it.Value(), GetDocumentDataFolder(it.Key()));
}



