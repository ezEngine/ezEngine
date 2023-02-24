#include <Foundation/Algorithm/Sorting.h>
#include <Foundation/Application/Application.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Memory/StackAllocator.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/UniquePtr.h>
#include <Texture/Image/Image.h>


class ezBgraMovieMakerApp : public ezApplication
{
private:
  ezString m_sSearchDir;
  ezString m_outputFilePath;
  bool m_bHadErrors;
  bool m_bHadSeriousWarnings;
  bool m_bHadWarnings;

public:
  typedef ezApplication SUPER;

  ezBgraMovieMakerApp()
    : ezApplication("BgraMovieMaker")
  {
    m_bHadErrors = false;
    m_bHadSeriousWarnings = false;
    m_bHadWarnings = false;
  }

  /// Makes sure the apps return value reflects whether there were any errors or warnings
  static void LogInspector(const ezLoggingEventData& eventData)
  {
    ezBgraMovieMakerApp* app = (ezBgraMovieMakerApp*)ezApplication::GetApplicationInstance();

    switch (eventData.m_EventType)
    {
      case ezLogMsgType::ErrorMsg:
        app->m_bHadErrors = true;
        break;
      case ezLogMsgType::SeriousWarningMsg:
        app->m_bHadSeriousWarnings = true;
        break;
      case ezLogMsgType::WarningMsg:
        app->m_bHadWarnings = true;
        break;

      default:
        break;
    }
  }

  virtual void AfterCoreSystemsStartup() override
  {
    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
    ezGlobalLog::AddLogWriter(LogInspector);

    if (GetArgumentCount() < 2)
      ezLog::Error("This tool requires at leas one command-line argument: An absolute path to the top-level folder of a library.");

    // Add the empty data directory to access files via absolute paths
    ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites).IgnoreResult();

    // pass the absolute path to the directory that should be scanned as the first parameter to this application
    ezStringBuilder sSearchDir;

    auto numArgs = GetArgumentCount();
    for (ezUInt32 argi = 1; argi < numArgs; argi++)
    {
      auto arg = ezStringView(GetArgument(argi));
      if (arg == "-o")
      {
        if (argi + 1 >= numArgs)
        {
          ezLog::Error("Missing argument for -o");
          return;
        }
        argi++;
        m_outputFilePath = GetArgument(argi);
      }
      else
      {
        if (sSearchDir.IsEmpty())
        {
          sSearchDir = arg;
          sSearchDir.MakeCleanPath();
        }
        else
        {
          ezLog::Error("Currently only one directory is supported for searching.");
        }
      }
    }

    if (!ezPathUtils::IsAbsolutePath(sSearchDir.GetData()))
    {
      ezStringBuilder absPath = ezOSFile::GetCurrentWorkingDirectory();
      absPath.AppendPath(sSearchDir);
      absPath.MakeCleanPath();
      sSearchDir = absPath;
    }

    if (!ezPathUtils::IsAbsolutePath(m_outputFilePath))
    {
      ezStringBuilder absPath = ezOSFile::GetCurrentWorkingDirectory();
      absPath.AppendPath(m_outputFilePath);
      absPath.MakeCleanPath();
      m_outputFilePath = absPath;
    }

    m_sSearchDir = sSearchDir;
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    if (m_bHadWarnings || m_bHadSeriousWarnings || m_bHadErrors)
    {
      ezLog::Warning("There have been errors or warnings, see log for details.");
    }

    if (m_bHadErrors || m_bHadSeriousWarnings)
      SetReturnCode(2);
    else if (m_bHadWarnings)
      SetReturnCode(1);
    else
      SetReturnCode(0);

    ezGlobalLog::RemoveLogWriter(LogInspector);
    ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  }

  virtual ezApplication::Execution Run() override
  {
    // something basic has gone wrong
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return ezApplication::Execution::Quit;

    ezStringBuilder search = m_sSearchDir;
    search.AppendPath("*.png");

    ezFileSystemIterator fileIt;
    fileIt.StartSearch(search, ezFileSystemIteratorFlags::ReportFiles);

    ezDynamicArray<ezString> files;
    for (; fileIt.IsValid(); fileIt.Next())
    {
      fileIt.GetStats().GetFullPath(search);
      search.MakeCleanPath();
      files.PushBack(search);
    }

    files.Sort();

    ezOSFile outputFile;
    if (outputFile.Open(m_outputFilePath, ezFileOpenMode::Write).Failed())
    {
      ezLog::Error("Failed to open output file {} for writing", m_outputFilePath);
      return ezApplication::Execution::Quit;
    }

    for (ezString& file : files)
    {
      ezImage img;
      if (img.LoadFrom(file).Failed())
      {
        ezLog::Error("failed to load file {}", file);
        break;
      }

      if (img.Convert(ezImageFormat::B8G8R8A8_UNORM).Failed())
      {
        ezLog::Error("Failed to convert file {}", file);
        break;
      }

      auto byteBlob = img.GetByteBlobPtr();
      if (outputFile.Write(byteBlob.GetPtr(), byteBlob.GetCount()).Failed())
      {
        ezLog::Error("Failed to write data to {}", m_outputFilePath);
        break;
      }
    }

    ezLog::Info("Successfully wrote rgba movie to {}", m_outputFilePath);

    return ezApplication::Execution::Quit;
  }
};

EZ_CONSOLEAPP_ENTRY_POINT(ezBgraMovieMakerApp);
