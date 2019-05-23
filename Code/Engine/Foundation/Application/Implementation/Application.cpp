#include <FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Logging/Log.h>

ezApplication::ezApplication(const char* szAppName)
    : m_iReturnCode(0)
    , m_uiArgumentCount(0)
    , m_ppArguments(nullptr)
    , m_bReportMemoryLeaks(true)
    , m_sAppName(szAppName)
{
}

ezApplication::~ezApplication() = default;

void ezApplication::SetApplicationName(const char* szAppName)
{
  m_sAppName = szAppName;
}

ezResult ezApplication::BeforeCoreSystemsStartup()
{
  if (ezFileSystem::DetectSdkRootDirectory().Failed())
  {
    ezLog::Error("Unable to find the SDK root directory. Mounting data directories may fail.");
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  ezRTTI::VerifyCorrectnessForAllTypes();
#endif

  if (ezCommandLineUtils::GetGlobalInstance()->GetBoolOption("-WaitForDebugger"))
  {
    while (!ezSystemInformation::IsDebuggerAttached())
    {
      ezThreadUtils::Sleep(ezTime::Milliseconds(1));
    }

    EZ_DEBUG_BREAK;
  }

  return EZ_SUCCESS;
}


void ezApplication::SetCommandLineArguments(ezUInt32 uiArgumentCount, const char** ppArguments)
{
  m_uiArgumentCount = uiArgumentCount;
  m_ppArguments = ppArguments;

  ezCommandLineUtils::GetGlobalInstance()->SetCommandLine(uiArgumentCount, ppArguments, ezCommandLineUtils::PreferOsArgs);
}


const char* ezApplication::GetArgument(ezUInt32 uiArgument) const
{
  EZ_ASSERT_DEV(uiArgument < m_uiArgumentCount, "There are only {0} arguments, cannot access argument {1}.", m_uiArgumentCount, uiArgument);

  return m_ppArguments[uiArgument];
}


void ezApplication::RequestQuit()
{
  m_bWasQuitRequested = true;
}


ezApplication* ezApplication::s_pApplicationInstance = nullptr;


