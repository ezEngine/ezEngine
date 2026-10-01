#include <EditorPluginSubstance/EditorPluginSubstancePCH.h>

#include <EditorPluginSubstance/Preferences/SubstancePreferences.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSubstancePreferences, 1, ezRTTIDefaultAllocator<ezSubstancePreferences>)
{
  EZ_BEGIN_PROPERTIES
  {
	  EZ_MEMBER_PROPERTY("InstallationPath", m_sInstallationPath),
	  EZ_MEMBER_PROPERTY("ToolTimeout", m_ToolTimeout)->AddAttributes(new ezDefaultValueAttribute(ezTime::Seconds(600)), new ezClampValueAttribute(ezTime::Seconds(10), ezVariant())),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  bool IsValidInstallationPath(ezStringView sPath)
  {
    if (sPath.IsEmpty())
      return false;

    ezStringBuilder path = sPath;
    path.AppendPath("sbscooker.exe");

    return path.IsAbsolutePath() && ezOSFile::ExistsFile(path);
  }
} // namespace

ezSubstancePreferences::ezSubstancePreferences()
  : ezPreferences(Domain::Application, "Substance")
{
  ezStringBuilder sPath;
  if (DetectInstallationPath(sPath).Succeeded())
  {
    m_sInstallationPath = sPath;
  }
}

ezResult ezSubstancePreferences::GetInstallationPath(ezStringBuilder& out_sPath) const
{
  if (IsValidInstallationPath(m_sInstallationPath))
  {
    out_sPath = m_sInstallationPath;
    return EZ_SUCCESS;
  }

  ezLog::Error("Installation of Substance Designer could not be located at '{}'. Please set the installation path in the Substance editor preferences.", m_sInstallationPath);
  return EZ_FAILURE;
}

ezResult ezSubstancePreferences::DetectInstallationPath(ezStringBuilder& out_sPath)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  ezStringBuilder sPath = "C:/Program Files/Allegorithmic/Substance Designer";
  if (IsValidInstallationPath(sPath))
  {
    out_sPath = sPath;
    return EZ_SUCCESS;
  }

  QSettings settings("\\HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\{e9e3d6d9-3023-41c7-b223-11d8fdd691b9}_is1", QSettings::NativeFormat);
  sPath = ezStringView(settings.value("InstallLocation").toString().toUtf8());

  if (IsValidInstallationPath(sPath))
  {
    out_sPath = sPath;
    out_sPath.MakeCleanPath();
    return EZ_SUCCESS;
  }
#endif

  EZ_IGNORE_UNUSED(out_sPath);
  return EZ_FAILURE;
}
