#include <FoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/MiniDumpUtils.h>
#include <Foundation/System/Process.h>
#include <Foundation/Time/Timestamp.h>

static void PrintHelper(const char* szString)
{
  ezLog::Printf("%s", szString);
}

//////////////////////////////////////////////////////////////////////////

ezCrashHandler* ezCrashHandler::s_pActiveHandler = nullptr;

ezCrashHandler::ezCrashHandler() = default;

ezCrashHandler::~ezCrashHandler()
{
  if (s_pActiveHandler == this)
  {
    SetCrashHandler(nullptr);
  }
}

ezCrashHandler* ezCrashHandler::GetCrashHandler()
{
  return s_pActiveHandler;
}

//////////////////////////////////////////////////////////////////////////

ezCrashHandler_WriteMiniDump ezCrashHandler_WriteMiniDump::g_Instance;

ezCrashHandler_WriteMiniDump::ezCrashHandler_WriteMiniDump() = default;

void ezCrashHandler_WriteMiniDump::SetFullDumpFilePath(const char* szFullAbsDumpFilePath)
{
  m_sDumpFilePath = szFullAbsDumpFilePath;
}

void ezCrashHandler_WriteMiniDump::SetDumpFilePath(const char* szAbsDirectoryPath, const char* szAppName, ezBitflags<PathFlags> flags)
{
  ezStringBuilder sOutputPath = szAbsDirectoryPath;

  if (flags.IsSet(PathFlags::AppendSubFolder))
  {
    sOutputPath.AppendPath("CrashDumps");
  }

  sOutputPath.AppendPath(szAppName);

  if (flags.IsSet(PathFlags::AppendDate))
  {
    const ezDateTime date = ezTimestamp::CurrentTimestamp();
    sOutputPath.AppendFormat("_{}", date);
  }

#if EZ_ENABLED(EZ_SUPPORTS_PROCESSES)
  if (flags.IsSet(PathFlags::AppendPID))
  {
    const ezUInt32 pid = ezProcess::GetCurrentProcessID();
    sOutputPath.AppendFormat("_{}", pid);
  }
#endif

  sOutputPath.Append(".dmp");

  SetFullDumpFilePath(sOutputPath);
}

void ezCrashHandler_WriteMiniDump::SetDumpFilePath(const char* szAppName, ezBitflags<PathFlags> flags)
{
  SetDumpFilePath(ezOSFile::GetApplicationDirectory(), szAppName, flags);
}

void ezCrashHandler_WriteMiniDump::HandleCrash(void* pOsSpecificData)
{
  if (!m_sDumpFilePath.IsEmpty())
  {
    if (ezMiniDumpUtils::LaunchMiniDumpTool(m_sDumpFilePath).Failed())
    {
      ezLog::Error("Could not launch MiniDumpTool, trying to write crash-dump from crashed process directly.");

      WriteOwnProcessMiniDump(pOsSpecificData);
    }
  }
  else
  {
    ezLog::Warning("ezCrashHandler_WriteMiniDump: No dump-file location specified.");
  }

  PrintStackTrace(pOsSpecificData);

  ezLog::Error("Application crashed. Crash-dump written to '{}'.", m_sDumpFilePath);
}

//////////////////////////////////////////////////////////////////////////

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/System/Implementation/Win/CrashHandler_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/System/Implementation/Posix/CrashHandler_posix.h>
#else
#  error "ezCrashHandler is not implemented on current platform"
#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_ExceptionHandler);
