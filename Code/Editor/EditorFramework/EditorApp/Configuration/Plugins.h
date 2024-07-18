#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Timestamp.h>

class ezOpenDdlWriter;
class ezOpenDdlReader;

/// \brief A plugin bundle lists all the files and information needed to get one feature plugin working
/// both in the editor and the runtime.
///
/// So it lists the editor DLLs, the runtime DLLs, all the additional transitive dependencies that need to be packaged,
/// and so on.
struct EZ_EDITORFRAMEWORK_DLL ezPluginBundle
{
  // State: user selectable per project
  bool m_bSelected = false; ///< whether this bundle is supposed to be used.
  bool m_bLoadCopy = false; ///< Whether the engine should load a copy of the runtime DLL, so that the original can be replaced and potentially reloaded.

  // Temp state:
  bool m_bMissing = false;
  ezTimestamp m_LastModificationTime;

  // General Bundle Description
  bool m_bMandatory = false;                        ///< if set, the bundle is always used and not even displayed in the UI
  ezString m_sDisplayName;                          ///< The string for displaying the bundle in UI
  ezString m_sDescription;                          ///< A proper description what this bundle is for, so that users know when to use it.
  ezHybridArray<ezString, 1> m_EditorPlugins;       ///< List of all the DLLs (without extension) to load into the editor process.
  ezHybridArray<ezString, 1> m_EditorEnginePlugins; ///< List of all the DLLs to load into the editor's engine process.
  ezHybridArray<ezString, 1> m_RuntimePlugins;      ///< List of all the DLLs to load into the runtime. These will also get packaged.
  ezHybridArray<ezString, 1> m_PackageDependencies; ///< Additional files to include in packages. E.g. indirect DLL dependencies.
  ezHybridArray<ezString, 1> m_RequiredBundles;     ///< The file names (without path or extension) of other bundles that are required for this bundle to work.
  ezHybridArray<ezString, 1> m_ExclusiveFeatures;   ///< If two bundles have the same string in this list, they can't be activated at the same time. So for example only one bundle with the feature 'Sound' or 'Physics' may be activated simultaneously. Only enforced by the UI.
  ezHybridArray<ezString, 1> m_EnabledInTemplates;  ///< In which project templates this plugin should be active by default.

  /// \brief Reads the bundle description, but not the state.
  ezResult ReadBundleFromDDL(ezOpenDdlReader& ref_ddl);

  /// \brief Writes only the bundle's state to a DDL file.
  void WriteStateToDDL(ezOpenDdlWriter& ref_ddl, const char* szOwnName) const;

  /// \brief Reads only the bundle's state from a DDL file.
  void ReadStateFromDDL(ezOpenDdlReader& ref_ddl, const char* szOwnName);

  /// \brief Checks whether two bundles have the same state.
  bool IsStateEqual(const ezPluginBundle& rhs) const
  {
    return m_bSelected == rhs.m_bSelected && m_bLoadCopy == rhs.m_bLoadCopy;
  }
};

/// \brief Contains multiple ezPluginBundle's.
struct EZ_EDITORFRAMEWORK_DLL ezPluginBundleSet
{
  ezMap<ezString, ezPluginBundle, ezCompareString_NoCase> m_Plugins;

  /// \brief Writes the state of all bundles to a DDL file.
  void WriteStateToDDL(ezOpenDdlWriter& ref_ddl) const;

  /// \brief Reads the state of all bundles from a DDL file.
  void ReadStateFromDDL(ezOpenDdlReader& ref_ddl);

  /// \brief Checks whether two bundle sets have the same state.
  bool IsStateEqual(const ezPluginBundleSet& rhs) const;
};
