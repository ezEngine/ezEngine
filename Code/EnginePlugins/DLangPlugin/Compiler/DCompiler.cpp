#include <DLangPlugin/DLangPluginPCH.h>

#include <DLangPlugin/Compiler/DCompiler.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/System/ProcessGroup.h>
#include <Foundation/Threading/Lock.h>

ezResult ezDLangCompiler::Initialize()
{
  if (m_bInitialized)
    return EZ_SUCCESS;

  EZ_LOG_BLOCK("DLangCompilerInit");

  EZ_SUCCEED_OR_RETURN(FindDmd());

  m_bInitialized = true;

  return EZ_SUCCESS;
}

void ezDLangCompiler::SetOutputBinary(const char* szOutputBin)
{
  ezStringBuilder tmp = szOutputBin;
  tmp.MakeCleanPath();
  m_sOutputBinary = tmp;
}

ezResult ezDLangCompiler::FindDmd()
{
  EZ_LOG_BLOCK("Find DMD");

  ezStringBuilder envPath = ezEnvironmentVariableUtils::GetValueString("Path");

  if (envPath.IsEmpty())
  {
    ezLog::Error("Failed to retrieve 'Path' environment variable.");
    return EZ_FAILURE;
  }

  ezHybridArray<ezStringView, 64> paths;
  envPath.Split(false, paths, ";");

  ezStringBuilder dmdPath;

  for (auto path : paths)
  {
    dmdPath = path;
    dmdPath.AppendPath("dmd.exe");
    dmdPath.MakeCleanPath();

    if (ezOSFile::ExistsFile(dmdPath))
    {
      m_sDmdPath = dmdPath;
      return EZ_SUCCESS;
    }
  }

  ezLog::Error("Could not find dmd.exe. Make sure it is installed its bin folder is added to the 'Path' environment variable.");
  return EZ_FAILURE;
}

ezResult ezDLangCompiler::CompileProjectLib()
{
  if (!m_bInitialized)
    return EZ_FAILURE;

  EZ_LOG_BLOCK("Compiling D project lib");

  if (!DetectSources())
  {
    if (m_SourceFiles.IsEmpty())
    {
      ezLog::Error("No D sources found.");
      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  if (ezOSFile::ExistsFile(m_sOutputBinary))
  {
    if (ezOSFile::DeleteFile(m_sOutputBinary).Failed())
    {
      ezLog::Error("Couldn't delete target DLL '{}'.", m_sOutputBinary);
      return EZ_FAILURE;
    }
  }

  ezProcessOptions po;
  po.m_sProcess = m_sDmdPath;
  po.m_bHideConsoleWindow = true;
  po.m_onStdOut = [](ezStringView text)
  {
    ezLog::Info("D: {}", text);
  };
  po.m_onStdError = [](ezStringView text)
  {
    ezLog::Error("D: {}", text);
  };
  po.AddArgument(ezFmt("-of{}", m_sOutputBinary));
  po.AddArgument("-debug"); // TODO build types
  po.AddArgument("-m64");
  po.AddArgument("-op"); // TODO keep ?
  po.AddArgument("-shared");
  po.AddArgument("-g");                                                                  // symbols
  po.AddArgument("D:/GitHub/ez/ezEngine/Output/Lib/WinVs2019Debug64/ezFoundation.lib");  // TODO
  po.AddArgument("D:/GitHub/ez/ezEngine/Output/Lib/WinVs2019Debug64/ezCore.lib");        // TODO
  po.AddArgument("D:/GitHub/ez/ezEngine/Output/Lib/WinVs2019Debug64/ezDLangPlugin.lib"); // TODO

  for (const auto& file : m_SourceFiles)
  {
    po.AddArgument(file);
  }

  ezStringBuilder cmd;
  po.BuildCommandLineString(cmd);
  cmd.Prepend(m_sDmdPath, " ");
  ezLog::Dev("DMD: '{}'", cmd);

  if (ezProcess::Execute(po).Failed())
  {
    ezLog::Error("Could not launch dmd.exe");
    return EZ_FAILURE;
  }

  // not sure why, but this doesn't finish, even though the compiler is done very quickly
  //ezProcessGroup pg;
  //if (pg.Launch(po).Failed())
  //{
  //  ezLog::Error("Could not launch dmd.exe");
  //  return EZ_FAILURE;
  //}

  //if (pg.WaitToFinish(ezTime::Seconds(10)).Failed())
  //{
  //  ezLog::Error("Timeout compiling D sources.");
  //  return EZ_FAILURE;
  //}

  return EZ_SUCCESS;
}

bool ezDLangCompiler::DetectSources()
{
  EZ_LOCK(ezFileSystem::GetMutex());

  ezSet<ezString> sources;

  ezTimestamp lastPluginUpdate;
  bool bSourcesModified = false;

  ezFileStats stat;
  if (ezOSFile::GetFileStats(m_sOutputBinary, stat).Succeeded())
  {
    lastPluginUpdate = stat.m_LastModificationTime;
  }

  for (ezUInt32 ddIdx = 0; ddIdx < ezFileSystem::GetNumDataDirectories(); ++ddIdx)
  {
    ezStringBuilder sDataDirPath = ezFileSystem::GetDataDirectory(ddIdx)->GetRedirectedDataDirectoryPath();
    sDataDirPath.MakeCleanPath();

    if (!sDataDirPath.IsAbsolutePath()) // ignore the DLang plugin directory
      continue;

    ezFileSystemIterator fsIt;
    fsIt.StartSearch(sDataDirPath, ezFileSystemIteratorFlags::ReportFilesRecursive);

    ezStringBuilder sFilePath;
    for (; fsIt.IsValid(); fsIt.Next())
    {
      if (!fsIt.GetStats().m_sName.EndsWith_NoCase(".d"))
        continue;

      fsIt.GetStats().GetFullPath(sFilePath);

      if (sFilePath.FindSubString("DLang/Templates") != nullptr)
        continue;

      //sFilePath.MakeRelativeTo(sDataDirPath).IgnoreResult();

      sources.Insert(sFilePath);

      if (!lastPluginUpdate.IsValid() || fsIt.GetStats().m_LastModificationTime.Compare(lastPluginUpdate, ezTimestamp::CompareMode::Newer))
      {
        bSourcesModified = true;
        lastPluginUpdate = fsIt.GetStats().m_LastModificationTime;
      }
    }
  }

  if (m_SourceFiles.GetCount() > sources.GetCount()) // a source file was removed
  {
    bSourcesModified = true;
  }

  m_SourceFiles.Swap(sources);
  return bSourcesModified;
}
