#include <Foundation/Application/Application.h>
#include <Foundation/IO/Archive/ArchiveBuilder.h>
#include <Foundation/IO/Archive/ArchiveReader.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/CommandLineOptions.h>

/* ArchiveTool command line options:

-out <path>
    Path to a file or folder.

    -out specifies the target to pack or unpack things to.
    For packing mode it has to be a file. The file will be overwritten, if it already exists.
    For unpacking, the target should be a folder (may or may not exist) into which the archives get extracted.

    If no -out is specified, it is determined to be where the input file is located.

-unpack <paths>
    One or multiple paths to ezArchive files that shall be extracted.

    Example:
      -unpack "path/to/file.ezArchive" "another/file.ezArchive"

-pack <paths>
    One or multiple paths to folders that shall be packed.

    Example:
      -pack "path/to/folder" "path/to/another/folder"

Description:
    -pack and -unpack can take multiple inputs to either aggregate multiple folders into one archive (pack)
    or to unpack multiple archives at the same time.

    If neither -pack nor -unpack is specified, the mode is detected automatically from the list of inputs.
    If all inputs are folders, the mode is 'pack'.
    If all inputs are files, the mode is 'unpack'.

Examples:
    ezArchiveTool.exe "C:/Stuff"
      Packs all data in "C:/Stuff" into "C:/Stuff.ezArchive"

    ezArchiveTool.exe "C:/Stuff" -out "C:/MyStuff.ezArchive"
      Packs all data in "C:/Stuff" into "C:/MyStuff.ezArchive"

    ezArchiveTool.exe "C:/Stuff.ezArchive"
      Unpacks all data from the archive into "C:/Stuff"

    ezArchiveTool.exe "C:/Stuff.ezArchive" -out "C:/MyStuff"
      Unpacks all data from the archive into "C:/MyStuff"
*/

ezCommandLineOptionPath opt_Out("_ArchiveTool", "-out", "\
Path to a file or folder.\n\
\n\
-out specifies the target to pack or unpack things to.\n\
For packing mode it has to be a file. The file will be overwritten, if it already exists.\n\
For unpacking, the target should be a folder (may or may not exist) into which the archives get extracted.\n\
\n\
If no -out is specified, it is determined to be where the input file is located.\n\
",
  "");

ezCommandLineOptionDoc opt_Unpack("_ArchiveTool", "-unpack", "<paths>", "\
One or multiple paths to ezArchive files that shall be extracted.\n\
\n\
Example:\n\
  -unpack \"path/to/file.ezArchive\" \"another/file.ezArchive\"\n\
",
  "");

ezCommandLineOptionDoc opt_Pack("_ArchiveTool", "-pack", "<paths>", "\
One or multiple paths to folders that shall be packed.\n\
\n\
Example:\n\
  -pack \"path/to/folder\" \"path/to/another/folder\"\n\
",
  "");

ezCommandLineOptionDoc opt_Desc("_ArchiveTool", "Description:", "", "\
-pack and -unpack can take multiple inputs to either aggregate multiple folders into one archive (pack)\n\
or to unpack multiple archives at the same time.\n\
\n\
If neither -pack nor -unpack is specified, the mode is detected automatically from the list of inputs.\n\
If all inputs are folders, the mode is 'pack'.\n\
If all inputs are files, the mode is 'unpack'.\n\
",
  "");

ezCommandLineOptionDoc opt_Examples("_ArchiveTool", "Examples:", "", "\
ezArchiveTool.exe \"C:/Stuff\"\n\
  Packs all data in \"C:/Stuff\" into \"C:/Stuff.ezArchive\"\n\
\n\
ezArchiveTool.exe \"C:/Stuff\" -out \"C:/MyStuff.ezArchive\"\n\
  Packs all data in \"C:/Stuff\" into \"C:/MyStuff.ezArchive\"\n\
\n\
ezArchiveTool.exe \"C:/Stuff.ezArchive\"\n\
  Unpacks all data from the archive into \"C:/Stuff\"\n\
\n\
ezArchiveTool.exe \"C:/Stuff.ezArchive\" -out \"C:/MyStuff\"\n\
  Unpacks all data from the archive into \"C:/MyStuff\"\n\
",
  "");

class ezArchiveBuilderImpl : public ezArchiveBuilder
{
protected:
  virtual void WriteFileResultCallback(ezUInt32 uiCurEntry, ezUInt32 uiMaxEntries, ezStringView sSourceFile, ezUInt64 uiSourceSize, ezUInt64 uiStoredSize, ezTime duration) const override
  {
    const ezUInt64 uiPercentage = (uiSourceSize == 0) ? 100 : (uiStoredSize * 100 / uiSourceSize);
    ezLog::Info(" [{}%%] {} ({}%%) - {}", ezArgU(100 * uiCurEntry / uiMaxEntries, 2), sSourceFile, uiPercentage, duration);
  }
};

class ezArchiveReaderImpl : public ezArchiveReader
{
public:
protected:
  virtual bool ExtractNextFileCallback(ezUInt32 uiCurEntry, ezUInt32 uiMaxEntries, ezStringView sSourceFile) const override
  {
    ezLog::Info(" [{}%%] {}", ezArgU(100 * uiCurEntry / uiMaxEntries, 2), sSourceFile);
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
  using SUPER = ezApplication;

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

  ezResult ParseArguments()
  {
    if (GetArgumentCount() <= 1)
    {
      ezLog::Error("No arguments given");
      return EZ_FAILURE;
    }

    ezCommandLineUtils& cmd = *ezCommandLineUtils::GetGlobalInstance();

    m_sOutput = opt_Out.GetOptionValue(ezCommandLineOption::LogMode::Always);

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
        m_sInputs.PushBack(cmd.GetAbsolutePathOption("-pack", a));

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
        m_sInputs.PushBack(cmd.GetAbsolutePathOption("-unpack", a));

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
        const ezStringView sArg = GetArgument(a);

        if (sArg.IsEqual_NoCase("-out"))
          break;

        m_sInputs.PushBack(ezOSFile::MakePathAbsoluteWithCWD(sArg));

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
    // Add the empty data directory to access files via absolute paths
    ezFileSystem::AddDataDirectory("", "App", ":", ezDataDirUsage::AllowWrites).IgnoreResult();

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

  static ezArchiveBuilder::InclusionMode PackFileCallback(ezStringView sFile)
  {
    const ezStringView ext = ezPathUtils::GetFileExtension(sFile);

    if (ext.IsEqual_NoCase("jpg") || ext.IsEqual_NoCase("jpeg") || ext.IsEqual_NoCase("png"))
      return ezArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("zip") || ext.IsEqual_NoCase("7z"))
      return ezArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("mp3") || ext.IsEqual_NoCase("ogg"))
      return ezArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("dds"))
      return ezArchiveBuilder::InclusionMode::Compress_zstd_fast;

    return ezArchiveBuilder::InclusionMode::Compress_zstd_average;
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

    m_sOutput = ezOSFile::MakePathAbsoluteWithCWD(m_sOutput);

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

      // if the file has a custom archive file extension, just register it as 'allowed'
      // we assume that the user only gives us files that are ezArchives
      if (!ezArchiveUtils::IsAcceptedArchiveFileExtensions(ezPathUtils::GetFileExtension(file)))
      {
        ezArchiveUtils::GetAcceptedArchiveFileExtensions().PushBack(ezPathUtils::GetFileExtension(file));
      }

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

  virtual Execution Run() override
  {
    {
      ezStringBuilder cmdHelp;
      if (ezCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, ezCommandLineOption::LogAvailableModes::IfHelpRequested, "_ArchiveTool"))
      {
        ezLog::Print(cmdHelp);
        return ezApplication::Execution::Quit;
      }
    }

    ezStopwatch sw;

    if (ParseArguments().Failed())
    {
      SetReturnCode(1);
      return ezApplication::Execution::Quit;
    }

    if (m_Mode == ArchiveMode::Pack)
    {
      if (Pack().Failed())
      {
        ezLog::Error("Packaging files failed");
        SetReturnCode(2);
      }

      ezLog::Success("Finished packing archive in {}", sw.GetRunningTotal());
      return ezApplication::Execution::Quit;
    }

    if (m_Mode == ArchiveMode::Unpack)
    {
      if (Unpack().Failed())
      {
        ezLog::Error("Extracting files failed");
        SetReturnCode(3);
      }

      ezLog::Success("Finished extracting archive in {}", sw.GetRunningTotal());
      return ezApplication::Execution::Quit;
    }

    ezLog::Error("Unknown mode");
    return ezApplication::Execution::Quit;
  }
};

EZ_APPLICATION_ENTRY_POINT(ezArchiveTool);
