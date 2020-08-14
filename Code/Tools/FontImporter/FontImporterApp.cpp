#include <FontImporterPCH.h>
#include <Core/Assets/AssetFileHeader.h>
#include <FontImporterApp.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>

ezFontImporterApp::ezFontImporterApp()
  : ezApplication("FontImporter")
{
}

ezResult ezFontImporterApp::BeforeCoreSystemsStartup()
{
  ezStartup::AddApplicationTag("tool");
  ezStartup::AddApplicationTag("fontimporter");

  return SUPER::BeforeCoreSystemsStartup();
}

void ezFontImporterApp::AfterCoreSystemsStartup()
{
  ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites);

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  m_FontImporter.Startup();
}

void ezFontImporterApp::BeforeCoreSystemsShutdown()
{
  m_FontImporter.Shutdown();

  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

ezApplication::ApplicationExecution ezFontImporterApp::Run()
{
  SetReturnCode(-1);

  if (ParseCommandLine().Failed())
    return ezApplication::ApplicationExecution::Quit;

  ezFontImportOptions importOptions;
  ezRawFont font;

  if (m_FontImporter.Import(m_sInputFile, importOptions, font).Failed())
  {
    ezLog::Error("Failed to import font file: {0}.");
  }

  if (WriteOutputFile(m_sOutputFile, font).Failed())
  {
    ezLog::Error("Failed to write imported font to '{0}'", m_sOutputFile);
    return ezApplication::ApplicationExecution::Quit;
  }
  else
  {
    ezLog::Success("Wrote imported font to '{}'", m_sOutputFile);
  }

  SetReturnCode(0);
  return ezApplication::ApplicationExecution::Quit;
}

ezResult ezFontImporterApp::ParseCommandLine()
{
  auto* pCmd = ezCommandLineUtils::GetGlobalInstance();

  if (pCmd->GetBoolOption("-help") || pCmd->GetBoolOption("--help") || pCmd->GetBoolOption("-h"))
  {
    ezLog::Info("ezFontImporter Help:");
    ezLog::Info("");
    ezLog::Info("  -out \"File.ezFont\"");
    ezLog::Info("     Absolute path to main output file");
    ezLog::Info("");
    ezLog::Info("  -in \"File\"");
    ezLog::Info("    Specifies input font file.");

    return EZ_FAILURE;
  }

  EZ_SUCCEED_OR_RETURN(ParseInputFile());
  EZ_SUCCEED_OR_RETURN(ParseOutputFile());

  return EZ_SUCCESS;
}

ezResult ezFontImporterApp::ParseInputFile()
{
  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();

  ParseFile("-in", m_sInputFile);
  return EZ_SUCCESS;
}

ezResult ezFontImporterApp::ParseOutputFile()
{
  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();

  ParseFile("-out", m_sOutputFile);
  return EZ_SUCCESS;
}

bool ezFontImporterApp::ParseFile(const char* szOption, ezString& result) const
{
  const auto pCmd = ezCommandLineUtils::GetGlobalInstance();
  result = pCmd->GetAbsolutePathOption(szOption);

  if (!result.IsEmpty())
  {
    ezLog::Info("'{}' file: '{}'", szOption, result);
    return true;
  }
  else
  {
    ezLog::Info("No '{}' file specified.", szOption);
    return false;
  }
}

ezResult ezFontImporterApp::WriteFontFile(ezStreamWriter& stream, const ezRawFont& font)
{
  if (font.Write(stream).Failed())
  {
    ezLog::Error("Failed to write data to the font file.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

ezResult ezFontImporterApp::WriteOutputFile(const char* szFile, const ezRawFont& font)
{
  ezDeferredFileWriter file;
  file.SetOutput(szFile);

  WriteFontFile(file, font);

  return file.Close();
}

EZ_CONSOLEAPP_ENTRY_POINT(ezFontImporterApp);
