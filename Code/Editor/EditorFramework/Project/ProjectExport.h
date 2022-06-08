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
  static ezStatus ClearTargetFolder(const char* szAbsFolderPath);
  static ezResult ScanFolder(ezSet<ezString>& out_Files, const char* szFolder, const ezPathPatternFilter& filter, ezProgress* pProgress, ezAssetCurator* pCurator);
  static ezResult CopyFiles(const char* szSrcFolder, const char* szDstFolder, const ezSet<ezString>& files, ezProgress* pProgress, ezProgressRange* pProgressRange, ezLogInterface* pLog);
  static void GatherGeneratedAssetManagerFiles(ezSet<ezString>& out_Files);
};
