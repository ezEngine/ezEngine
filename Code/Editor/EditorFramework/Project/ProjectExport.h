#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>

struct ezPathPatternFilter;
class ezAssetCurator;
class ezApplicationFileSystemConfig;
class ezPlatformProfile;
class ezProgressRange;

//////////////////////////////////////////////////////////////////////////

struct EZ_EDITORFRAMEWORK_DLL ezProjectExport
{
  static ezResult ExportProject(const char* szTargetDirectory, const ezPlatformProfile* pPlatformProfile, const ezApplicationFileSystemConfig& dataDirs);

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
  static ezResult CreateLaunchConfig(const ezDynamicArray<ezString>& sceneFiles, const char* szTargetDirectory);
  static ezResult GatherGeneratedAssetFiles(ezSet<ezString>& out_Files, const char* szProjectDirectory);
};
