#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineOptions.h>

ezApplication::ezApplication(ezStringView sAppName)
  : m_sAppName(sAppName)
{
}

ezApplication::~ezApplication() = default;

void ezApplication::SetApplicationName(ezStringView sAppName)
{
  m_sAppName = sAppName;
}

ezCommandLineOptionBool opt_WaitForDebugger("app", "-WaitForDebugger", "If specified, the application will wait at startup until a debugger is attached.", false);

ezResult ezApplication::BeforeCoreSystemsStartup()
{
  if (ezFileSystem::DetectSdkRootDirectory().Failed())
  {
    ezLog::Error("Unable to find the SDK root directory. Mounting data directories may fail.");
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  ezRTTI::VerifyCorrectnessForAllTypes();
#endif

  if (opt_WaitForDebugger.GetOptionValue(ezCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    while (!ezSystemInformation::IsDebuggerAttached())
    {
      ezThreadUtils::Sleep(ezTime::MakeFromMilliseconds(1));
    }

    EZ_DEBUG_BREAK;
  }

  return EZ_SUCCESS;
}


void ezApplication::SetCommandLineArguments(ezUInt32 uiArgumentCount, const char** pArguments)
{
  m_uiArgumentCount = uiArgumentCount;
  m_pArguments = pArguments;

  ezCommandLineUtils::GetGlobalInstance()->SetCommandLine(uiArgumentCount, pArguments, ezCommandLineUtils::PreferOsArgs);
}


const char* ezApplication::GetArgument(ezUInt32 uiArgument) const
{
  EZ_ASSERT_DEV(uiArgument < m_uiArgumentCount, "There are only {0} arguments, cannot access argument {1}.", m_uiArgumentCount, uiArgument);

  return m_pArguments[uiArgument];
}


void ezApplication::RequestApplicationQuit()
{
  m_bWasQuitRequested = true;
}


ezApplication* ezApplication::s_pApplicationInstance = nullptr;
