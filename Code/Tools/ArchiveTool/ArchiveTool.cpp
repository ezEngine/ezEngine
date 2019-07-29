#include <Foundation/Application/Application.h>
#include <Foundation/IO/Archive/ArchiveBuilder.h>
#include <Foundation/IO/Archive/ArchiveReader.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/SystemInformation.h>

/* ezArchiveTool command line options:

Options to be specific:

-pack "path/to/folder" "path/to/another/folder" ...
-unpack "path/to/file.ezArchive" "another/file.ezArchive"
-out "path/to/file/or/folder"

-pack and -unpack can take multiple inputs to either aggregate multiple folders into one archive (pack)
or to unpack multiple archives at the same time.

-out specifies the target to pack or unpack things to. For packing mode it has to be a file.
The file will be overwritten, if it already exists.
For unpacking the target should be a folder (may or may not exist) into which the archives get extracted.

If no -out is specified, it is determined to be where the input file is located.

If neither -pack nor -unpack is specified, the mode is detected automatically from the list of inputs.
If all inputs are folders, mode is going to be 'pack'.
If all inputs are files, mode is going to be 'unpack'.

Examples:

ezArchiveTool.exe "C:\Stuff"
  will pack all data in "C:\Stuff" into "C:\Stuff.ezArchive"

ezArchiveTool.exe "C:\Stuff" -out "C:\MyStuff.ezArchive"
  will pack all data in "C:\Stuff" into "C:\MyStuff.ezArchive"

ezArchiveTool.exe "C:\Stuff.ezArchive"
  will unpack all data from the archive into "C:\Stuff"

ezArchiveTool.exe "C:\Stuff.ezArchive" -out "C:\MyStuff"
  will unpack all data from the archive into "C:\MyStuff"

*/

class ezArchiveBuilderImpl : public ezArchiveBuilder
{
public:
protected:
  virtual bool WriteNextFileCallback(ezUInt32 uiCurEntry, ezUInt32 uiMaxEntries, const char* szSourceFile) const override
  {
    ezLog::Info(" [{}%%] {}", ezArgU(100 * uiCurEntry / uiMaxEntries, 2), szSourceFile);
    return true;
  }


  virtual bool WriteFileProgressCallback(ezUInt64 bytesWritten, ezUInt64 bytesTotal) const override
  {
    // ezLog::Dev("   {}%%", ezArgU(100 * bytesWritten / bytesTotal));
    return true;
  }
};

class ezArchiveReaderImpl : public ezArchiveReader
{
public:
protected:
  virtual bool ExtractNextFileCallback(ezUInt32 uiCurEntry, ezUInt32 uiMaxEntries, const char* szSourceFile) const override
  {
    ezLog::Info(" [{}%%] {}", ezArgU(100 * uiCurEntry / uiMaxEntries, 2), szSourceFile);
    return true;
  }


  virtual bool ExtractFileProgressCallback(ezUInt64 bytesWritten, ezUInt64 bytesTotal) const override
  {
    // ezLog::Dev("   {}%%", ezArgU(100 * bytesWritten / bytesTotal));
    return true;
  }
};

class ezArchiveTool : public ezApplication
{
public:
  typedef ezApplication SUPER;

  enum class ArchiveMode
  {
    Auto,
    Pack,
    Unpack,
  };

  ArchiveMode m_Mode = ArchiveMode::Auto;

  ezDynamicArray<ezString> m_sInputs;
  ezString m_sOutput;

  ezArchiveTool()
    : ezApplication("ArchiveTool")
  {
  }

  ezStringBuilder m_sTmpPath;

  const ezStringBuilder& AbsPath(const char* path)
  {
    m_sTmpPath = path;
    m_sTmpPath.MakeCleanPath();

    if (m_sTmpPath.IsRelativePath())
    {
      m_sTmpPath.Prepend('/');
      m_sTmpPath.Prepend(ezOSFile::GetCurrentWorkingDirectory());
      m_sTmpPath.MakeCleanPath();
    }

    return m_sTmpPath;
  }

  ezResult ParseArguments()
  {
    if (GetArgumentCount() <= 1)
    {
      ezLog::Error("No arguments given");
      return EZ_FAILURE;
    }

    ezCommandLineUtils& cmd = *ezCommandLineUtils::GetGlobalInstance();

    m_sOutput = cmd.GetStringOption("-out");

    ezStringBuilder path;

    if (cmd.GetStringOptionArguments("-pack") > 0)
    {
      m_Mode = ArchiveMode::Pack;
      const ezUInt32 args = cmd.GetStringOptionArguments("-pack");

      if (args == 0)
      {
        ezLog::Error("-pack option expects at least one argument");
        return EZ_FAILURE;
      }

      for (ezUInt32 a = 0; a < args; ++a)
      {
        m_sInputs.PushBack(AbsPath(cmd.GetStringOption("-pack", a)));

        if (!ezOSFile::ExistsDirectory(m_sInputs.PeekBack()))
        {
          ezLog::Error("-pack input path is not a valid directory: '{}'", m_sInputs.PeekBack());
          return EZ_FAILURE;
        }
      }
    }
    else if (cmd.GetStringOptionArguments("-unpack") > 0)
    {
      m_Mode = ArchiveMode::Unpack;
      const ezUInt32 args = cmd.GetStringOptionArguments("-unpack");

      if (args == 0)
      {
        ezLog::Error("-unpack option expects at least one argument");
        return EZ_FAILURE;
      }

      for (ezUInt32 a = 0; a < args; ++a)
      {
        m_sInputs.PushBack(AbsPath(cmd.GetStringOption("-unpack", a)));

        if (!ezOSFile::ExistsFile(m_sInputs.PeekBack()))
        {
          ezLog::Error("-unpack input file does not exist: '{}'", m_sInputs.PeekBack());
          return EZ_FAILURE;
        }
      }
    }
    else
    {
      bool bInputsFolders = true;
      bool bInputsFiles = true;

      for (ezUInt32 a = 1; a < GetArgumentCount(); ++a)
      {
        const char* szArg = GetArgument(a);

        if (ezStringUtils::IsEqual_NoCase(szArg, "-out"))
          break;

        m_sInputs.PushBack(AbsPath(szArg));

        if (!ezOSFile::ExistsDirectory(m_sInputs.PeekBack()))
          bInputsFolders = false;
        if (!ezOSFile::ExistsFile(m_sInputs.PeekBack()))
          bInputsFiles = false;
      }

      if (bInputsFolders && !bInputsFiles)
      {
        m_Mode = ArchiveMode::Pack;
      }
      else if (bInputsFiles && !bInputsFolders)
      {
        m_Mode = ArchiveMode::Unpack;
      }
      else
      {
        ezLog::Error("Inputs are ambiguous. Specify only folders for packing or only files for unpacking. Use -out as last argument to "
                     "specify a target.");
        return EZ_FAILURE;
      }
    }

    ezLog::Info("Mode is: {}", m_Mode == ArchiveMode::Pack ? "pack" : "unpack");
    ezLog::Info("Inputs:");

    for (const auto& input : m_sInputs)
    {
      ezLog::Info("  '{}'", input);
    }

    ezLog::Info("Output: '{}'", m_sOutput);

    return EZ_SUCCESS;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    // Add standard folder factory
    ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);

    // Add the empty data directory to access files via absolute paths
    ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites);

    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    // prevent further output during shutdown
    ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

    SUPER::BeforeCoreSystemsShutdown();
  }

  static ezArchiveBuilder::InclusionMode PackFileCallback(const char* szFile)
  {
    const ezStringView ext = ezPathUtils::GetFileExtension(szFile);

    if (ext.IsEqual_NoCase("jpg") || ext.IsEqual_NoCase("jpeg") || ext.IsEqual_NoCase("png"))
      return ezArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("zip") || ext.IsEqual_NoCase("7z"))
      return ezArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("mp3") || ext.IsEqual_NoCase("ogg"))
      return ezArchiveBuilder::InclusionMode::Uncompressed;

    return ezArchiveBuilder::InclusionMode::Compress_zstd;
  }

  ezResult Pack()
  {
    ezArchiveBuilderImpl archive;

    for (const auto& folder : m_sInputs)
    {
      archive.AddFolder(folder, ezArchiveCompressionMode::Compressed_zstd, PackFileCallback);
    }

    if (m_sOutput.IsEmpty())
    {
      ezStringBuilder sArchive = m_sInputs[0];
      sArchive.Append(".ezArchive");

      m_sOutput = sArchive;
    }

    m_sOutput = AbsPath(m_sOutput);

    ezLog::Info("Writing archive to '{}'", m_sOutput);
    if (archive.WriteArchive(m_sOutput).Failed())
    {
      ezLog::Error("Failed to write the ezArchive");

      return EZ_FAILURE;
    }

    return EZ_SUCCESS;
  }

  ezResult Unpack()
  {
    for (const auto& file : m_sInputs)
    {
      ezLog::Info("Extracting archive '{}'", file);

      ezArchiveReaderImpl reader;
      EZ_SUCCEED_OR_RETURN(reader.OpenArchive(file));

      ezStringBuilder sOutput = m_sOutput;

      if (sOutput.IsEmpty())
      {
        sOutput = file;
        sOutput.RemoveFileExtension();
      }

      if (reader.ExtractAllFiles(sOutput).Failed())
      {
        ezLog::Error("File extraction failed.");
        return EZ_FAILURE;
      }
    }

    return EZ_SUCCESS;
  }

  virtual ApplicationExecution Run() override
  {
    if (ParseArguments().Failed())
    {
      SetReturnCode(1);
      return ezApplication::Quit;
    }

    if (m_Mode == ArchiveMode::Pack)
    {
      if (Pack().Failed())
      {
        ezLog::Error("Packaging files failed");
        SetReturnCode(2);
      }

      return ezApplication::Quit;
    }

    if (m_Mode == ArchiveMode::Unpack)
    {
      if (Unpack().Failed())
      {
        ezLog::Error("Extracting files failed");
        SetReturnCode(2);
      }

      return ezApplication::Quit;
    }

    ezLog::Error("Unknown mode");
    return ezApplication::Quit;
  }
};

EZ_CONSOLEAPP_ENTRY_POINT(ezArchiveTool);
