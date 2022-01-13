#include "DLangCodeGenTool.h"
#include "DLangGenerator.h"
#include <Foundation/IO/FileSystem/FileReader.h>

ezCommandLineOptionPath opt_Xml("_DLangCodeGenTool", "-xml", "Path to an XML input file.", "");

void DLangCodeGenTool::AfterCoreSystemsStartup()
{
  // Add the empty data directory to access files via absolute paths
  ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites).IgnoreResult();

  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
}

void DLangCodeGenTool::BeforeCoreSystemsShutdown()
{
  // prevent further output during shutdown
  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

ezApplication::Execution DLangCodeGenTool::Run()
{
  {
    ezStringBuilder cmdHelp;
    if (ezCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, ezCommandLineOption::LogAvailableModes::IfHelpRequested, "_ArchiveTool"))
    {
      ezLog::Print(cmdHelp);
      return ezApplication::Execution::Quit;
    }
  }

  if (ParseArguments().Failed())
  {
    SetReturnCode(1);
    return ezApplication::Execution::Quit;
  }

  ezFileReader file;
  if (file.Open(m_sXmlFile).Failed())
  {
    ezLog::Error("Could not read XML file '{0}'", m_sXmlFile);
    SetReturnCode(1);
  }

  if (m_Structure.ParseXML(file).Failed())
  {
    SetReturnCode(4);
    return ezApplication::Execution::Quit;
  }

  m_pGenerator = EZ_DEFAULT_NEW(DLangGenerator);
  m_pGenerator->SetStructure(m_Structure);

  //m_pGenerator->WhitelistType("ezGameObjectId");

  FindTargetFiles();
  CleanTargetFiles();

  m_Phase = Phase::GatherInfo;
  ProcessAllFiles();

  m_Phase = Phase::GenerateCode;
  ProcessAllFiles();

  m_pGenerator->GenerateStructure("ezGameObjectId", TargetType::Value).IgnoreResult();

  return ezApplication::Execution::Quit;
}

ezResult DLangCodeGenTool::ParseArguments()
{
  ezStringBuilder xmlPath;
  ezFileSystem::ResolveSpecialDirectory(">sdk", xmlPath).IgnoreResult();
  xmlPath.AppendPath("Code/Tools/DLangCodeGenTool/CppExtraction/ez.xml");

  opt_Xml.SetDefault(xmlPath);
  m_sXmlFile = opt_Xml.GetOptionValue(ezCommandLineOption::LogMode::Always);

  if (!ezFileSystem::ExistsFile(m_sXmlFile))
  {
    ezLog::Error("Input XML file does not exist.");
    return EZ_FAILURE;
  }

  return EZ_SUCCESS;
}

EZ_CONSOLEAPP_ENTRY_POINT(DLangCodeGenTool);
