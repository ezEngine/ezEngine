#pragma once

#include <EditorPluginSubstance/EditorPluginSubstanceDLL.h>

#include <EditorFramework/Preferences/Preferences.h>

class EZ_EDITORPLUGINSUBSTANCE_DLL ezSubstancePreferences : public ezPreferences
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSubstancePreferences, ezPreferences);

public:
  ezSubstancePreferences();

  /// \brief Returns the configured Substance Designer installation folder. Fails if the folder does not contain the Substance tools.
  ezResult GetInstallationPath(ezStringBuilder& out_sPath) const;

  /// \brief Timeout in seconds for running sbscooker and sbsrender.
  ezUInt32 GetToolTimeout() const { return static_cast<ezUInt32>(m_ToolTimeout.GetSeconds()); }

  /// \brief Tries to locate the Substance Designer installation folder on this machine.
  static ezResult DetectInstallationPath(ezStringBuilder& out_sPath);

private:
  ezString m_sInstallationPath;
  ezTime m_ToolTimeout = ezTime::Seconds(600);
};
