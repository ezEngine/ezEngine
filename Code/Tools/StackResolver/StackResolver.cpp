#include <Foundation/Application/Application.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/System/StackTracer.h>

#include <Foundation/Platform/Win/Utils/IncludeWindows.h>

#include <DbgHelp.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/ConversionUtils.h>

struct Module
{
  ezString m_sFilePath;
  ezUInt64 m_uiBaseAddress;
  ezUInt32 m_uiSize;
};

struct Stackframe
{
  ezUInt32 m_uiModuleIndex = 0xFFFFFFFF;
  ezUInt32 m_uiLineNumber = 0;
  ezString m_sFilename;
  ezString m_sSymbol;
};

ezCommandLineOptionString opt_ModuleList("_app", "-ModuleList", "List of modules as a string in this format:\n\n\
File1Path?File1BaseAddressHEX?File1Size|File2Path?File2BaseAddressHEX?File2Size|...\n\n\
For example:\n\
  $[A]/app.exe?7FF7E5540000?106496|$[S]/System32/KERNELBASE.dll?7FFE2B780000?2920448\n\n\
  $[A] represents the application directory and will be adjusted as necessary.\n\
  $[S] represents the system root directory and will be adjusted as necessary.",
  "");
ezCommandLineOptionString opt_Callstack("_app", "-Callstack", "Callstack in this format:\n\n7FFE2DD6CE74|7FFE2B7AAA86|7FFE034C22D1", "");

ezCommandLineOptionEnum opt_OutputFormat("_app", "-Format", "How to output the resolved callstack.", "Text=0|JSON=1", 0);

ezCommandLineOptionPath opt_OutputFile("_app", "-File", "The target file where to write the output to.\nIf left empty, the output is printed to the console.", "");

class ezStackResolver : public ezApplication
{
public:
  using SUPER = ezApplication;

  ezStackResolver()
    : ezApplication("ezStackResolver")
  {
  }

  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

  virtual void Run() override;

  ezResult LoadModules();
  ezResult ParseModules();
  ezResult ParseCallstack();

  void ResolveStackFrames();
  void FormatAsText(ezStringBuilder& ref_sOutput);
  void FormatAsJSON(ezStringBuilder& ref_sOutput);

  HANDLE m_hProcess;
  ezDynamicArray<Module> m_Modules;
  ezDynamicArray<ezUInt64> m_Callstack;
  ezDynamicArray<Stackframe> m_Stackframes;
  ezStringBuilder m_SystemRootDir;
  ezStringBuilder m_ApplicationDir;
};

void ezStackResolver::AfterCoreSystemsStartup()
{
  ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);

  m_Modules.Reserve(128);
  m_Callstack.Reserve(128);
  m_Stackframes.Reserve(128);
}

void ezStackResolver::BeforeCoreSystemsShutdown()
{
  ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
  ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
}

ezResult ezStackResolver::ParseModules()
{
  const ezStringBuilder sModules = opt_ModuleList.GetOptionValue(ezCommandLineOption::LogMode::Never);

  ezDynamicArray<ezStringView> parts;
  sModules.Split(false, parts, "|");

  for (ezStringView sModView : parts)
  {
    ezStringBuilder sMod = sModView;
    ezDynamicArray<ezStringView> parts2;
    sMod.Split(false, parts2, "?");

    ezUInt64 base;
    if (ezConversionUtils::ConvertHexStringToUInt64(parts2[1], base).Failed())
    {
      ezLog::Error("Failed to convert HEX string '{}' to UINT64", parts2[1]);
      return EZ_FAILURE;
    }

    ezStringBuilder sSize = parts2[2];
    ezUInt32 size;
    if (ezConversionUtils::StringToUInt(sSize, size).Failed())
    {
      ezLog::Error("Failed to convert string '{}' to UINT32", sSize);
      return EZ_FAILURE;
    }

    ezStringBuilder sModuleName = parts2[0];
    sModuleName.ReplaceFirst_NoCase("$[S]", m_SystemRootDir);
    sModuleName.ReplaceFirst_NoCase("$[A]", m_ApplicationDir);
    sModuleName.MakeCleanPath();

    auto& mod = m_Modules.ExpandAndGetRef();
    mod.m_sFilePath = sModuleName;
    mod.m_uiBaseAddress = base;
    mod.m_uiSize = size;
  }

  return EZ_SUCCESS;
}

ezResult ezStackResolver::ParseCallstack()
{
  ezStringBuilder sCallstack = opt_Callstack.GetOptionValue(ezCommandLineOption::LogMode::Never);

  ezDynamicArray<ezStringView> parts;
  sCallstack.Split(false, parts, "|");
  for (ezStringView sModView : parts)
  {
    ezUInt64 base;
    if (ezConversionUtils::ConvertHexStringToUInt64(sModView, base).Failed())
    {
      ezLog::Error("Failed to convert HEX string '{}' to UINT64", sModView);
      return EZ_FAILURE;
    }

    m_Callstack.PushBack(base);
  }

  return EZ_SUCCESS;
}

ezResult ezStackResolver::LoadModules()
{
  if (SymInitialize(m_hProcess, nullptr, FALSE) != TRUE) // TODO specify PDB search path as second parameter?
  {
    ezLog::Error("SymInitialize failed");
    return EZ_FAILURE;
  }

  for (const auto& curModule : m_Modules)
  {
    if (SymLoadModuleExW(m_hProcess, nullptr, ezStringWChar(curModule.m_sFilePath), nullptr, curModule.m_uiBaseAddress, curModule.m_uiSize, nullptr, 0) == 0)
    {
      ezLog::Warning("Couldn't load module '{}'", curModule.m_sFilePath);
    }
    else
    {
      ezLog::Success("Loaded module '{}'", curModule.m_sFilePath);
    }
  }

  return EZ_SUCCESS;
}

void ezStackResolver::ResolveStackFrames()
{
  ezStringBuilder tmp;

  char buffer[1024];
  for (ezUInt32 i = 0; i < m_Callstack.GetCount(); i++)
  {
    DWORD64 symbolAddress = m_Callstack[i];

    _SYMBOL_INFOW& symbolInfo = *(_SYMBOL_INFOW*)buffer;
    ezMemoryUtils::ZeroFill(&symbolInfo, 1);
    symbolInfo.SizeOfStruct = sizeof(_SYMBOL_INFOW);
    symbolInfo.MaxNameLen = (EZ_ARRAY_SIZE(buffer) - symbolInfo.SizeOfStruct) / sizeof(WCHAR);

    DWORD64 displacement = 0;
    BOOL result = SymFromAddrW(m_hProcess, symbolAddress, &displacement, &symbolInfo);
    if (!result)
    {
      wcscpy_s(symbolInfo.Name, symbolInfo.MaxNameLen, L"<Unknown>");
    }

    IMAGEHLP_LINEW64 lineInfo;
    DWORD displacement2 = static_cast<DWORD>(displacement);
    ezMemoryUtils::ZeroFill(&lineInfo, 1);
    lineInfo.SizeOfStruct = sizeof(lineInfo);
    SymGetLineFromAddrW64(m_hProcess, symbolAddress, &displacement2, &lineInfo);

    auto& frame = m_Stackframes.ExpandAndGetRef();

    for (ezUInt32 modIndex = 0; modIndex < m_Modules.GetCount(); modIndex++)
    {
      if (m_Modules[modIndex].m_uiBaseAddress == (ezUInt64)symbolInfo.ModBase)
      {
        frame.m_uiModuleIndex = modIndex;
        break;
      }
    }

    frame.m_uiLineNumber = (ezUInt32)lineInfo.LineNumber;
    frame.m_sSymbol = ezStringUtf8(symbolInfo.Name).GetView();

    tmp = ezStringUtf8(lineInfo.FileName).GetView();
    tmp.MakeCleanPath();
    frame.m_sFilename = tmp;
  }
}

void ezStackResolver::FormatAsText(ezStringBuilder& ref_sOutput)
{
  ezLog::Info("Formatting callstack as text.");

  for (const auto& frame : m_Stackframes)
  {
    ezStringView sModuleName = "<unknown module>";

    if (frame.m_uiModuleIndex < m_Modules.GetCount())
    {
      sModuleName = m_Modules[frame.m_uiModuleIndex].m_sFilePath;
    }

    ezStringView sFileName = "<unknown file>";
    if (!frame.m_sFilename.IsEmpty())
    {
      sFileName = frame.m_sFilename;
    }

    ezStringView sSymbol = "<unknown symbol>";
    if (!frame.m_sSymbol.IsEmpty())
    {
      sSymbol = frame.m_sSymbol;
    }

    ref_sOutput.AppendFormat("[][{}] {}({}): '{}'\n", sModuleName, sFileName, frame.m_uiLineNumber, sSymbol);
  }
}

void ezStackResolver::FormatAsJSON(ezStringBuilder& ref_sOutput)
{
  ezLog::Info("Formatting callstack as JSON.");

  ezContiguousMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);

  ezStandardJSONWriter json;
  json.SetOutputStream(&writer);
  json.SetWhitespaceMode(ezJSONWriter::WhitespaceMode::LessIndentation);

  json.BeginObject();
  json.BeginArray("Stackframes");

  for (const auto& frame : m_Stackframes)
  {
    ezStringView sModuleName = "<unknown>";

    if (frame.m_uiModuleIndex < m_Modules.GetCount())
    {
      sModuleName = m_Modules[frame.m_uiModuleIndex].m_sFilePath;
    }

    ezStringView sFileName = "<unknown>";
    if (!frame.m_sFilename.IsEmpty())
    {
      sFileName = frame.m_sFilename;
    }

    ezStringView sSymbol = "<unknown>";
    if (!frame.m_sSymbol.IsEmpty())
    {
      sSymbol = frame.m_sSymbol;
    }

    json.BeginObject();
    json.AddVariableString("Module", sModuleName);
    json.AddVariableString("File", sFileName);
    json.AddVariableUInt32("Line", frame.m_uiLineNumber);
    json.AddVariableString("Symbol", sSymbol);
    json.EndObject();
  }

  json.EndArray();
  json.EndObject();

  ezStringView text((const char*)storage.GetData(), storage.GetStorageSize32());

  ref_sOutput.Append(text);
}

void ezStackResolver::Run()
{
  if (ezCommandLineOption::LogAvailableOptions(ezCommandLineOption::LogAvailableModes::IfHelpRequested, "_app"))
  {
    RequestApplicationQuit();
    return;
  }

  ezString sMissingOpt;
  if (ezCommandLineOption::RequireOptions("-ModuleList;-Callstack", &sMissingOpt).Failed())
  {
    ezLog::Error("Command line option '{}' was not specified.", sMissingOpt);

    ezCommandLineOption::LogAvailableOptions(ezCommandLineOption::LogAvailableModes::Always, "_app");
    RequestApplicationQuit();
    return;
  }

  m_hProcess = GetCurrentProcess();

  m_ApplicationDir = ezOSFile::GetApplicationDirectory();
  m_ApplicationDir.MakeCleanPath();
  m_ApplicationDir.Trim("", "/");

  m_SystemRootDir = ezEnvironmentVariableUtils::GetValueString("SystemRoot");
  m_SystemRootDir.MakeCleanPath();
  m_SystemRootDir.Trim("", "/");

  if (ParseModules().Failed())
  {
    RequestApplicationQuit();
    return;
  }

  if (ParseCallstack().Failed())
  {
    RequestApplicationQuit();
    return;
  }

  if (LoadModules().Failed())
  {
    RequestApplicationQuit();
    return;
  }

  ResolveStackFrames();

  ezStringBuilder output;

  if (opt_OutputFormat.GetOptionValue(ezCommandLineOption::LogMode::Never) == 0)
  {
    FormatAsText(output);
  }
  else if (opt_OutputFormat.GetOptionValue(ezCommandLineOption::LogMode::Never) == 1)
  {
    FormatAsJSON(output);
  }

  if (opt_OutputFile.IsOptionSpecified())
  {
    ezLog::Info("Writing output to '{}'.", opt_OutputFile.GetOptionValue(ezCommandLineOption::LogMode::Never));

    ezOSFile file;
    if (file.Open(opt_OutputFile.GetOptionValue(ezCommandLineOption::LogMode::Never), ezFileOpenMode::Write).Failed())
    {
      ezLog::Error("Could not open file for writing: '{}'", opt_OutputFile.GetOptionValue(ezCommandLineOption::LogMode::Never));
      RequestApplicationQuit();
      return;
    }

    file.Write(output.GetData(), output.GetElementCount()).IgnoreResult();
  }
  else
  {
    ezLog::Info("Writing output to console.");

    EZ_LOG_BLOCK("Resolved callstack");

    ezDynamicArray<ezStringView> lines;
    output.Split(true, lines, "\n");

    for (auto l : lines)
    {
      ezLog::Info("{}", l);
    }
  }

  RequestApplicationQuit();
}

EZ_APPLICATION_ENTRY_POINT(ezStackResolver);
