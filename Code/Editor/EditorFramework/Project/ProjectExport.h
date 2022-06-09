#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>

struct ezPathPatternFilter;
class ezAssetCurator;

/// \brief A log interface implementation that immediately writes all log messages to a file
class EZ_EDITORFRAMEWORK_DLL ezLogSystemToFile : public ezLogInterface
{
  ezOSFile m_File;

public:
  ezLogSystemToFile();
  ~ezLogSystemToFile();

  ezResult Open(const char* szAbsFile);

  virtual void HandleLogMessage(const ezLoggingEventData& le) override;
};

//////////////////////////////////////////////////////////////////////////

struct EZ_EDITORFRAMEWORK_DLL ezProjectExport
{
  struct DataDirectory
  {
    ezString m_sTargetDirPath;
    ezString m_sTargetDirRootName;
    ezSet<ezString> m_Files;
  };

  using DirectoryMapping = ezMap<ezString, DataDirectory>;

  static ezResult ClearTargetFolder(const char* szAbsFolderPath);
  static ezResult ScanFolder(ezSet<ezString>& out_Files, const char* szFolder, const ezPathPatternFilter& filter, ezAssetCurator* pCurator);
  static ezResult CopyFiles(const char* szSrcFolder, const char* szDstFolder, const ezSet<ezString>& files, ezProgressRange* pProgressRange);
  static void GatherGeneratedAssetManagerFiles(ezSet<ezString>& out_Files);
  static ezResult CreateExportFilterFile(const char* szExpectedFile, const char* szFallbackFile);
  static ezResult ReadExportFilters(ezPathPatternFilter& out_DataFilter, ezPathPatternFilter& out_BinariesFilter, const char* szPlatformProfileName);
  static ezResult CreateDataDirectoryDDL(const DirectoryMapping& mapping, const char* szTargetDirectory);
  static ezResult GatherAssetLookupTableFiles(DirectoryMapping& mapping, const ezApplicationFileSystemConfig& dirConfig, const char* szPlatformProfileName);
  static ezResult ScanDataDirectories(DirectoryMapping& mapping, const ezApplicationFileSystemConfig& dirConfig, const ezPathPatternFilter& dataFilter);
  static ezResult CopyAllFiles(DirectoryMapping& mapping, const char* szTargetDirectory);
  static ezResult GatherBinaries(DirectoryMapping& mapping, const ezPathPatternFilter& filter);
};
