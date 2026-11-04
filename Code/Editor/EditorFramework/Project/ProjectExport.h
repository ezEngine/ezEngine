#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/Status.h>

struct ezPathPatternFilter;
class ezAssetCurator;
class ezApplicationFileSystemConfig;
class ezPlatformProfile;
class ezProgressRange;

//////////////////////////////////////////////////////////////////////////

/// The steps around ezProjectExport::ExportProjectComplete(), all of which can be skipped.
///
/// The defaults are what the 'Export Project' dialog starts out with, ie. do everything.
struct EZ_EDITORFRAMEWORK_DLL ezProjectExportOptions
{
  /// Build the project's C++ plugin first, so that the exported binaries match the current code.
  /// Has no effect on a project without C++ code.
  bool m_bCompileCppPlugin = true;

  /// Transform all assets first. Without this, assets that were modified since the last transform are
  /// exported in their previous state, and assets never transformed for the active profile are missing
  /// from the export entirely.
  bool m_bTransformAssets = true;

  /// Run the project's post-export script, see ezProjectExport::FindPostExportScript(). Has no effect on a
  /// project without such a script. When disabled, multiple scripts for this platform aren't an error either.
  bool m_bRunPostExportScript = true;
};

/// What ezProjectExport::ExportProjectComplete() hands back to a caller that presents the result itself.
struct ezProjectExportReport
{
  ezString m_sLog;              ///< Everything that the export logged.
  ezString m_sProblems;         ///< Only the errors and warnings from m_sLog.

  ezUInt32 m_uiNumErrors = 0;   ///< Can be non-zero for a successful export, for instance when a single file couldn't be copied.
  ezUInt32 m_uiNumWarnings = 0; ///< Includes serious warnings.

  ezUInt32 m_uiNumFiles = 0;    ///< Number of files in the target directory. Only set when the export succeeded.
  ezUInt64 m_uiTotalBytes = 0;  ///< Size of all files in the target directory. Only set when the export succeeded.
};

struct EZ_EDITORFRAMEWORK_DLL ezProjectExport
{
  /// Runs the full export, as triggered by the 'Export Project' dialog.
  ///
  /// Does the optional steps in ezProjectExportOptions, then ExportProject() for the currently active
  /// asset profile and the editor's data directory configuration. The target directory is *cleared*
  /// first - see ExportProject().
  ///
  /// If out_pReport is not null, everything that the export logs is captured and returned through it. In that
  /// case it does not show up in the editor's log window, so the returned status and the report are the only
  /// indication of problems a caller gets. If out_pReport is null, the messages go to the regular log.
  ///
  /// A returned success doesn't mean that there were no problems, see ezProjectExportReport::m_uiNumErrors.
  ///
  /// If the project has a post-export script (see FindPostExportScript()), it is executed after a successful
  /// ExportProject(). This blocks until the script finishes and can't be canceled. If the script can't be started
  /// or returns a non-zero exit code, the export fails, but the exported files stay in the target directory.
  /// The script's stdout is logged as info, stderr as warnings, both only after the script has finished.
  static ezStatus ExportProjectComplete(ezStringView sTargetDirectory, const ezProjectExportOptions& options, ezProjectExportReport* out_pReport = nullptr);

  /// Finds the script that the active project wants to run on the exported files, for instance to remove files.
  ///
  /// The script is ':project/Editor/PostExport.<ext>', which isn't exported itself, because of the default data
  /// export filter. The supported extensions are 'ps1', 'py' and 'bat' on Windows, and 'sh', 'py' and 'ps1' elsewhere.
  /// A script with another extension is ignored, so a project can have one script per platform.
  ///
  /// The script runs with the target directory as its working directory and gets the environment variables
  /// EZ_EXPORT_DIR, EZ_PROJECT_DIR and EZ_ASSET_PROFILE.
  ///
  /// out_sScriptFile is set to the absolute path, or to an empty string if there is no script. It is an error if
  /// there are multiple scripts for this platform, since it wouldn't be clear which one is meant.
  static ezStatus FindPostExportScript(ezString& out_sScriptFile);


  /// Exports the project to szTargetDirectory.
  ///
  /// .bat files for launching the exported project are written to the target directory. If the project has a
  /// custom game executable (see FindCustomGameExecutable()), a single script for that executable is created,
  /// otherwise one script per exported scene, which launches the scene in ezPlayer.
  static ezResult ExportProject(const char* szTargetDirectory, const ezPlatformProfile* pPlatformProfile, const ezApplicationFileSystemConfig& dataDirs);

  /// Returns the file name of the project's own game executable, or an empty string if there is none.
  ///
  /// The executable is the one generated by the C++ project generation, ie. it is named after the plugin name
  /// in ':project/Editor/CppProject.ddl'. An empty string is returned when the project has no C++ code, or when
  /// the executable hasn't been built yet.
  static ezString FindCustomGameExecutable();

private:
  struct DataDirectory
  {
    ezString m_sTargetDirPath;
    ezString m_sTargetDirRootName;
    ezSet<ezString> m_Files;
  };

  using DirectoryMapping = ezMap<ezString, DataDirectory>;

  static ezResult ClearTargetFolder(const char* szAbsFolderPath);
  static ezResult ScanFolder(ezSet<ezString>& out_Files, const char* szFolder, const ezPathPatternFilter& filter, ezAssetCurator* pCurator, ezDynamicArray<ezString>* pSceneFiles, const ezPlatformProfile* pPlatformProfile);
  static ezResult CopyFiles(const char* szSrcFolder, const char* szDstFolder, const ezSet<ezString>& files, ezProgressRange* pProgressRange);
  static ezResult GatherGeneratedAssetManagerFiles(ezSet<ezString>& out_Files);
  static ezResult CreateExportFilterFile(const char* szExpectedFile, const char* szFallbackFile);
  static ezResult ReadExportFilters(ezPathPatternFilter& out_DataFilter, ezPathPatternFilter& out_BinariesFilter, const ezPlatformProfile* pPlatformProfile);
  static ezResult CreateDataDirectoryDDL(const DirectoryMapping& mapping, const char* szTargetDirectory);
  static ezResult GatherAssetLookupTableFiles(DirectoryMapping& mapping, const ezApplicationFileSystemConfig& dirConfig, const ezPlatformProfile* pPlatformProfile);
  static ezResult ScanDataDirectories(DirectoryMapping& mapping, const ezApplicationFileSystemConfig& dirConfig, const ezPathPatternFilter& dataFilter, ezDynamicArray<ezString>* pSceneFiles, const ezPlatformProfile* pPlatformProfile);
  static ezResult CopyAllFiles(DirectoryMapping& mapping, const char* szTargetDirectory);
  static ezResult GatherBinaries(DirectoryMapping& mapping, const ezPathPatternFilter& filter);
  static ezResult CreateLaunchConfig(const ezDynamicArray<ezString>& sceneFiles, const char* szTargetDirectory, ezStringView sCustomGameExecutable);
  static ezResult GatherGeneratedAssetFiles(ezSet<ezString>& out_Files, const char* szProjectDirectory);
  static void AddPackageDependenciesToFileList(DirectoryMapping& ref_fileList);
  static ezResult RunPostExportScript(ezStringView sScriptFile, ezStringView sTargetDirectory);
};
