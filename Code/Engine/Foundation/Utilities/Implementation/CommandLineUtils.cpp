#include <Foundation/PCH.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>

static ezCommandLineUtils g_pCmdLineInstance;

ezCommandLineUtils* ezCommandLineUtils::GetInstance()
{
  return &g_pCmdLineInstance;
}

void ezCommandLineUtils::SetCommandLine(ezUInt32 argc, const char** argv)
{
  m_Commands.Clear();

  // skip the first parameter, it contains the executable path
  if (argc <= 1)
    return;

  m_Commands.Reserve(argc - 1);

  for (ezUInt32 i = 1; i < argc; ++i)
    m_Commands.PushBack(argv[i]);
}

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

void ezCommandLineUtils::SetCommandLine()
{
 int argc = 0;

 LPWSTR* argvw = CommandLineToArgvW(GetCommandLineW(), &argc);

 EZ_ASSERT_RELEASE(argvw != nullptr, "CommandLineToArgvW failed");

 ezArrayPtr<ezStringUtf8> ArgvUtf8 = EZ_DEFAULT_NEW_ARRAY(ezStringUtf8, argc);
 ezArrayPtr<const char*> argv = EZ_DEFAULT_NEW_ARRAY(const char*, argc);

 for (ezInt32 i = 0; i < argc; ++i)
 {
   ArgvUtf8[i] = argvw[i];
   argv[i] = ArgvUtf8[i].GetData();
 }

 SetCommandLine(argc, argv.GetPtr());


 EZ_DEFAULT_DELETE_ARRAY(ArgvUtf8);
 EZ_DEFAULT_DELETE_ARRAY(argv);
 LocalFree(argvw);
}

#elif EZ_ENABLED(EZ_PLATFORM_OSX)
  // Not implemented on OSX.
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
  // Not implemented on Linux.
#else
  #error "ezCommandLineUtils::SetCommandLine(): Abstraction missing."
#endif

ezUInt32 ezCommandLineUtils::GetParameterCount() const
{
  return m_Commands.GetCount();
}

const char* ezCommandLineUtils::GetParameter(ezUInt32 uiParam) const
{
  return m_Commands[uiParam].GetData();
}

ezInt32 ezCommandLineUtils::GetOptionIndex(const char* szOption, bool bCaseSensitive) const
{
  EZ_ASSERT_DEV(ezStringUtils::StartsWith(szOption, "-"), "All command line option names must start with a hyphen (e.g. -file)");

  for (ezUInt32 i = 0; i < m_Commands.GetCount(); ++i)
  {
    if (( bCaseSensitive && m_Commands[i].IsEqual(szOption)) ||
        (!bCaseSensitive && m_Commands[i].IsEqual_NoCase(szOption)))
      return i;
  }

  return -1;
}

ezUInt32 ezCommandLineUtils::GetStringOptionArguments(const char* szOption, bool bCaseSensitive) const
{
  const ezInt32 iIndex = GetOptionIndex(szOption, bCaseSensitive);

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

const char* ezCommandLineUtils::GetStringOption(const char* szOption, ezUInt32 uiArgument, const char* szDefault, bool bCaseSensitive) const
{
  const ezInt32 iIndex = GetOptionIndex(szOption, bCaseSensitive);

  // not found -> no parameters
  if (iIndex < 0)
    return szDefault;

  ezUInt32 uiParamCount = 0;

  for (ezUInt32 uiParam = iIndex + 1; uiParam < m_Commands.GetCount(); ++uiParam)
  {
    if (m_Commands[uiParam].StartsWith("-")) // next command is the next option -> not enough parameters
      return szDefault;

    // found the right one, return it
    if (uiParamCount == uiArgument)
      return m_Commands[uiParam].GetData();

    ++uiParamCount;
  }

  return szDefault;
}

bool ezCommandLineUtils::GetBoolOption(const char* szOption, bool bDefault, bool bCaseSensitive) const
{
  const ezInt32 iIndex = GetOptionIndex(szOption, bCaseSensitive);

  if (iIndex < 0)
    return bDefault;

  if (iIndex + 1 == m_Commands.GetCount()) // last command, treat this as 'on'
    return true;

  if (m_Commands[iIndex + 1].StartsWith("-")) // next command is the next option -> treat this as 'on' as well
    return true;

  // otherwise try to convert the next option to a boolean
  bool bRes = bDefault;
  ezConversionUtils::StringToBool(m_Commands[iIndex + 1].GetData(), bRes);

  return bRes;
}

ezInt32 ezCommandLineUtils::GetIntOption(const char* szOption, ezInt32 iDefault, bool bCaseSensitive) const
{
  const ezInt32 iIndex = GetOptionIndex(szOption, bCaseSensitive);

  if (iIndex < 0)
    return iDefault;

  if (iIndex + 1 == m_Commands.GetCount()) // last command
    return iDefault;

  if (m_Commands[iIndex + 1].StartsWith("-")) // next command is the next option
    return iDefault;

  // otherwise try to convert the next option to an integer
  ezInt32 iRes = iDefault;
  ezConversionUtils::StringToInt(m_Commands[iIndex + 1].GetData(), iRes);

  return iRes;
}

double ezCommandLineUtils::GetFloatOption(const char* szOption, double fDefault, bool bCaseSensitive) const
{
  const ezInt32 iIndex = GetOptionIndex(szOption, bCaseSensitive);

  if (iIndex < 0)
    return fDefault;

  if (iIndex + 1 == m_Commands.GetCount()) // last command
    return fDefault;

  if (m_Commands[iIndex + 1].StartsWith("-")) // next command is the next option
    return fDefault;

  // otherwise try to convert the next option to an integer
  double fRes = fDefault;
  ezConversionUtils::StringToFloat(m_Commands[iIndex + 1].GetData(), fRes);

  return fRes;
}



EZ_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_CommandLineUtils);

