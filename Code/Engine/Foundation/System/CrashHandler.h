#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>

/// \brief Helper class to manage the top level exception handler.
///
/// A default exception handler is provided but not set by default.
/// The default implementation will write the exception and callstack to the output
/// and create a memory dump using WriteDump that create a dump file in the folder
/// specified via SetExceptionHandler.

/// \brief This class allows to hook into the OS top-level exception handler to handle application crashes
///
/// Derive from this class to implement custom behavior. Call ezCrashHandler::SetCrashHandler() to
/// register which instance to use.
///
/// For typical use-cases use ezCrashHandler_WriteMiniDump::g_Instance.
class EZ_FOUNDATION_DLL ezCrashHandler
{
public:
  ezCrashHandler();
  virtual ~ezCrashHandler();

  static void SetCrashHandler(ezCrashHandler* pHandler);
  static ezCrashHandler* GetCrashHandler();

  virtual void HandleCrash(void* pOsSpecificData) = 0;

private:
  static ezCrashHandler* s_pActiveHandler;
};

/// \brief A default implementation of ezCrashHandler that tries to write a mini-dump and prints the callstack.
///
/// To use it, call ezCrashHandler::SetCrashHandler(&ezCrashHandler_WriteMiniDump::g_Instance);
/// Do not forget to also specify the dump-file path, otherwise writing dump-files is skipped.
class EZ_FOUNDATION_DLL ezCrashHandler_WriteMiniDump : public ezCrashHandler
{
public:
  static ezCrashHandler_WriteMiniDump g_Instance;

  struct PathFlags
  {
    typedef ezUInt8 StorageType;

    enum Enum
    {
      AppendDate = EZ_BIT(0),      ///< Whether to append the current date to the crash-dump file (YYYY-MM-DD_HH-MM-SS)
      AppendSubFolder = EZ_BIT(1), ///< Whether to append "CrashDump" as a sub-folder
      AppendPID = EZ_BIT(2),       ///< Whether to append the process ID to the crash-dump file

      Default = AppendDate | AppendSubFolder | AppendPID
    };

    struct Bits
    {
      StorageType AppendDate : 1;
      StorageType AppendSubFolder : 1;
      StorageType AppendPID : 1;
    };
  };

public:
  ezCrashHandler_WriteMiniDump();

  /// \brief Sets the raw path for the dump-file to write
  void SetFullDumpFilePath(const char* szFullAbsDumpFilePath);

  /// \brief Sets the dump-file path to "{szAbsDirectoryPath}/{szAppName}_{cur-date}.tmp"
  void SetDumpFilePath(const char* szAbsDirectoryPath, const char* szAppName, ezBitflags<PathFlags> flags = PathFlags::Default);

  /// \brief Sets the dump-file path to "{ezOSFile::GetApplicationDirectory()}/{szAppName}_{cur-date}.tmp"
  void SetDumpFilePath(const char* szAppName, ezBitflags<PathFlags> flags = PathFlags::Default);

  virtual void HandleCrash(void* pOsSpecificData) override;

protected:
  virtual bool WriteOwnProcessMiniDump(void* pOsSpecificData);
  virtual void PrintStackTrace(void* pOsSpecificData);

  ezString m_sDumpFilePath;
};

EZ_DECLARE_FLAGS_OPERATORS(ezCrashHandler_WriteMiniDump::PathFlags);
