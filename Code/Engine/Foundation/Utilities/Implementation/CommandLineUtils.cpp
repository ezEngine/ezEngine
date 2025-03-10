#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <shellapi.h>
#endif

static ezCommandLineUtils g_pCmdLineInstance;

ezCommandLineUtils* ezCommandLineUtils::GetGlobalInstance()
{
  return &g_pCmdLineInstance;
}

void ezCommandLineUtils::SplitCommandLineString(const char* szCommandString, bool bAddExecutableDir, ezDynamicArray<ezString>& out_args, ezDynamicArray<const char*>& out_argsV)
{
  // Add application dir as first argument as customary on other platforms.
  if (bAddExecutableDir)
  {
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    wchar_t moduleFilename[256];
    GetModuleFileNameW(nullptr, moduleFilename, 256);
    out_args.PushBack(ezStringUtf8(moduleFilename).GetData());
#else
    EZ_ASSERT_NOT_IMPLEMENTED;
#endif
  }

  // Simple args splitting. Not as powerful as Win32's CommandLineToArgvW.
  const char* currentChar = szCommandString;
  const char* lastEnd = currentChar;
  bool inQuotes = false;
  while (*currentChar != '\0')
  {
    if (*currentChar == '\"')
      inQuotes = !inQuotes;
    else if (*currentChar == ' ' && !inQuotes)
    {
      ezStringBuilder path = ezStringView(lastEnd, currentChar);
      path.Trim(" \"");
      out_args.PushBack(path);
      lastEnd = currentChar + 1;
    }
    ezUnicodeUtils::MoveToNextUtf8(currentChar).IgnoreResult();
  }

  out_argsV.Reserve(out_argsV.GetCount());
  for (ezString& str : out_args)
    out_argsV.PushBack(str.GetData());
}

void ezCommandLineUtils::SetCommandLine(ezUInt32 uiArgc, const char** pArgv, ArgMode mode /*= UseArgcArgv*/)
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  if (mode == ArgMode::PreferOsArgs)
  {
    SetCommandLine();
    return;
  }
#else
  EZ_IGNORE_UNUSED(mode);
#endif

  m_Commands.Clear();
  m_Commands.Reserve(uiArgc);

  for (ezUInt32 i = 0; i < uiArgc; ++i)
    m_Commands.PushBack(pArgv[i]);
}

void ezCommandLineUtils::SetCommandLine(ezArrayPtr<ezString> commands)
{
  m_Commands = commands;
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)

void ezCommandLineUtils::SetCommandLine()
{
  int argc = 0;

  LPWSTR* argvw = CommandLineToArgvW(::GetCommandLineW(), &argc);

  EZ_ASSERT_RELEASE(argvw != nullptr, "CommandLineToArgvW failed");

  ezArrayPtr<ezStringUtf8> ArgvUtf8 = EZ_DEFAULT_NEW_ARRAY(ezStringUtf8, argc);
  ezArrayPtr<const char*> argv = EZ_DEFAULT_NEW_ARRAY(const char*, argc);

  for (ezInt32 i = 0; i < argc; ++i)
  {
    ArgvUtf8[i] = argvw[i];
    argv[i] = ArgvUtf8[i].GetData();
  }

  SetCommandLine(argc, argv.GetPtr(), ArgMode::UseArgcArgv);


  EZ_DEFAULT_DELETE_ARRAY(ArgvUtf8);
  EZ_DEFAULT_DELETE_ARRAY(argv);
  LocalFree(argvw);
}

#endif

const ezDynamicArray<ezString>& ezCommandLineUtils::GetCommandLineArray() const
{
  return m_Commands;
}

ezString ezCommandLineUtils::GetCommandLineString() const
{
  ezStringBuilder commandLine;
  for (const ezString& command : m_Commands)
  {
    if (commandLine.IsEmpty())
    {
      commandLine.Append(command.GetView());
    }
    else
    {
      commandLine.Append(" ", command);
    }
  }
  return commandLine;
}

ezUInt32 ezCommandLineUtils::GetParameterCount() const
{
  return m_Commands.GetCount();
}

const ezString& ezCommandLineUtils::GetParameter(ezUInt32 uiParam) const
{
  return m_Commands[uiParam];
}

ezInt32 ezCommandLineUtils::GetOptionIndex(ezStringView sOption, bool bCaseSensitive) const
{
  EZ_ASSERT_DEV(sOption.StartsWith("-"), "All command line option names must start with a hyphen (e.g. -file)");

  for (ezUInt32 i = 0; i < m_Commands.GetCount(); ++i)
  {
    if ((bCaseSensitive && m_Commands[i].IsEqual(sOption)) || (!bCaseSensitive && m_Commands[i].IsEqual_NoCase(sOption)))
      return i;
  }

  return -1;
}

bool ezCommandLineUtils::HasOption(ezStringView sOption, bool bCaseSensitive /*= false*/) const
{
  return GetOptionIndex(sOption, bCaseSensitive) >= 0;
}

ezUInt32 ezCommandLineUtils::GetStringOptionArguments(ezStringView sOption, bool bCaseSensitive) const
{
  const ezInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  // not found -> no parameters
  if (iIndex < 0)
    return 0;

  ezUInt32 uiParamCount = 0;

  for (ezUInt32 uiParam = iIndex + 1; uiParam < m_Commands.GetCount(); ++uiParam)
  {
    if (m_Commands[uiParam].StartsWith("-")) // next command is the next option -> no parameters
      break;

    ++uiParamCount;
  }

  return uiParamCount;
}

ezStringView ezCommandLineUtils::GetStringOption(ezStringView sOption, ezUInt32 uiArgument, ezStringView sDefault, bool bCaseSensitive) const
{
  const ezInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  // not found -> no parameters
  if (iIndex < 0)
    return sDefault;

  ezUInt32 uiParamCount = 0;

  for (ezUInt32 uiParam = iIndex + 1; uiParam < m_Commands.GetCount(); ++uiParam)
  {
    if (m_Commands[uiParam].StartsWith("-")) // next command is the next option -> not enough parameters
      return sDefault;

    // found the right one, return it
    if (uiParamCount == uiArgument)
    {
      // We trim " as this is automatically done on Windows when parsing command line arguments and this will make it behave the same on Linux.
      ezStringView sData = m_Commands[uiParam].GetView();
      sData.Trim("\"");
      return sData;
    }
    ++uiParamCount;
  }

  return sDefault;
}

const ezString ezCommandLineUtils::GetAbsolutePathOption(ezStringView sOption, ezUInt32 uiArgument /*= 0*/, ezStringView sDefault /*= {} */, bool bCaseSensitive /*= false*/) const
{
  ezStringView sPath = GetStringOption(sOption, uiArgument, sDefault, bCaseSensitive);
  if (sPath.IsEmpty())
    return sPath;

  return ezOSFile::MakePathAbsoluteWithCWD(sPath);
}

bool ezCommandLineUtils::GetBoolOption(ezStringView sOption, bool bDefault, bool bCaseSensitive) const
{
  const ezInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return bDefault;

  const ezUInt32 uiIndex = iIndex;
  if (uiIndex + 1 == m_Commands.GetCount())    // last command, treat this as 'on'
    return true;

  if (m_Commands[uiIndex + 1].StartsWith("-")) // next command is the next option -> treat this as 'on' as well
    return true;

  // otherwise try to convert the next option to a boolean
  bool bRes = bDefault;
  ezConversionUtils::StringToBool(m_Commands[uiIndex + 1].GetData(), bRes).IgnoreResult();

  return bRes;
}

ezInt32 ezCommandLineUtils::GetIntOption(ezStringView sOption, ezInt32 iDefault, bool bCaseSensitive) const
{
  const ezInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return iDefault;

  const ezUInt32 uiIndex = iIndex;
  if (uiIndex + 1 == m_Commands.GetCount()) // last command
    return iDefault;

  // try to convert the next option to a number
  ezInt32 iRes = iDefault;
  ezConversionUtils::StringToInt(m_Commands[uiIndex + 1].GetData(), iRes).IgnoreResult();

  return iRes;
}

ezUInt32 ezCommandLineUtils::GetUIntOption(ezStringView sOption, ezUInt32 uiDefault, bool bCaseSensitive) const
{
  const ezInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return uiDefault;

  const ezUInt32 uiIndex = iIndex;
  if (uiIndex + 1 == m_Commands.GetCount()) // last command
    return uiDefault;

  // try to convert the next option to a number
  ezUInt32 uiRes = uiDefault;
  ezConversionUtils::StringToUInt(m_Commands[uiIndex + 1].GetData(), uiRes).IgnoreResult();

  return uiRes;
}

double ezCommandLineUtils::GetFloatOption(ezStringView sOption, double fDefault, bool bCaseSensitive) const
{
  const ezInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return fDefault;

  const ezUInt32 uiIndex = iIndex;
  if (uiIndex + 1 == m_Commands.GetCount()) // last command
    return fDefault;

  // try to convert the next option to a number
  double fRes = fDefault;
  ezConversionUtils::StringToFloat(m_Commands[uiIndex + 1].GetData(), fRes).IgnoreResult();

  return fRes;
}

void ezCommandLineUtils::InjectCustomArgument(ezStringView sArgument)
{
  m_Commands.PushBack(sArgument);
}
