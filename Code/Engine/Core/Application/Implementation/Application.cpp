#include <PCH.h>

#include <Core/Application/Application.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Reflection/Reflection.h>

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

void ezApplication::BeforeCoreSystemsStartup()
{
  if (ezFileSystem::DetectSdkRootDirectory().Failed())
  {
    ezLog::Error("Unable to find the SDK root directory. Mounting data directories may fail.");
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  ezRTTI::VerifyCorrectnessForAllTypes();
#endif
}


void ezApplication::SetCommandLineArguments(ezUInt32 uiArgumentCount, const char** ppArguments)
{
  m_uiArgumentCount = uiArgumentCount;
  m_ppArguments = ppArguments;
  ezCommandLineUtils::GetGlobalInstance()->SetCommandLine(uiArgumentCount, ppArguments);
}


const char* ezApplication::GetArgument(ezUInt32 uiArgument) const
{
  EZ_ASSERT_DEV(uiArgument < m_uiArgumentCount, "There are only {0} arguments, cannot access argument {1}.", m_uiArgumentCount, uiArgument);

  return m_ppArguments[uiArgument];
}

ezApplication* ezApplication::s_pApplicationInstance = nullptr;

EZ_STATICLINK_FILE(Core, Core_Application_Implementation_Application);
