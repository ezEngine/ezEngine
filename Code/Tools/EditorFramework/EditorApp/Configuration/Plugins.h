#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

/// \brief Holds information about a plugin. Used for editor and engine plugins, where the user can configure whether to load them or not.
struct EZ_EDITORFRAMEWORK_DLL ezPluginInfo
{
  bool m_bAvailable = false; // exists on disk
  bool m_bActive = false; // currently loaded into the process
  bool m_bToBeLoaded = false; // supposed to be loaded into the process next restart
  bool m_bLoadCopy = false;

  bool operator==(const ezPluginInfo& rhs) const { return m_bAvailable == rhs.m_bAvailable && m_bActive == rhs.m_bActive && m_bToBeLoaded == rhs.m_bToBeLoaded && m_bLoadCopy == rhs.m_bLoadCopy; }
  bool operator!=(const ezPluginInfo& rhs) const { return !(*this == rhs); }
};

/// \brief Describes a set of plugins. Name is the plugin file name used to load it through ezPlugin, ezPluginInfo contains details about the loading state.
struct EZ_EDITORFRAMEWORK_DLL ezPluginSet
{
  ezMap<ezString, ezPluginInfo> m_Plugins;

  bool operator==(const ezPluginSet& rhs) const { return m_Plugins == rhs.m_Plugins; }
  bool operator!=(const ezPluginSet& rhs) const { return !(*this == rhs); }
};
