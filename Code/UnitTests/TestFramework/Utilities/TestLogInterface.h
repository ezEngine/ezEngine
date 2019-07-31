#pragma once
#include <Foundation/Logging/Log.h>
#include <TestFramework/TestFrameworkDLL.h>

/// \brief An ezLogInterface that expects and handles error messages during test runs. Can be
/// used to ensure that expected error messages are produced by the tested functionality.
/// Expected error messages are not passed on and do not cause tests to fail.
class EZ_TEST_DLL ezTestLogInterface : public ezLogInterface
{
public:
  ezTestLogInterface() = default;
  ~ezTestLogInterface();
  virtual void HandleLogMessage(const ezLoggingEventData& le) override;

  /// \brief Add expected message. Will fail the test when the expected message is not
  /// encountered. Can take an optional count, if messages are expected multiple times
  void ExpectMessage(const char* msg, ezLogMsgType::Enum type = ezLogMsgType::All, ezInt32 count = 1);

  /// \brief Set the log interface that unhandled messages are forwarded to.
  void SetParentLog(ezLogInterface* pInterface) { m_pParentLog = pInterface; }

private:
  ezLogInterface* m_pParentLog = nullptr;

  struct ExpectedMsg
  {
    ezInt32 m_iCount = 0;
    ezString m_sMsgSubString;
    ezLogMsgType::Enum m_Type = ezLogMsgType::All;
  };

  mutable ezMutex m_Mutex;
  ezHybridArray<ExpectedMsg, 8> m_expectedMessages;
};

/// \brief A class that sets a custom ezTestLogInterface as the thread local default log system,
/// and resets the previous system when it goes out of scope. The test version passes the previous
/// ezLogInterface on to the ezTestLogInterface to enable passing on unhandled messages.
///
/// If bCatchMessagesGlobally is false, the system only intercepts messages on the current thread.
/// If bCatchMessagesGlobally is true, it will also intercept messages from other threads, as long as they
/// go through ezGlobalLog. See ezGlobalLog::SetGlobalLogOverride().
class EZ_TEST_DLL ezTestLogSystemScope : public ezLogSystemScope
{
public:
  explicit ezTestLogSystemScope(ezTestLogInterface* pInterface, bool bCatchMessagesGlobally = false)
    : ezLogSystemScope(pInterface)
  {
    m_bCatchMessagesGlobally = bCatchMessagesGlobally;
    pInterface->SetParentLog(m_pPrevious);

    if (m_bCatchMessagesGlobally)
    {
      ezGlobalLog::SetGlobalLogOverride(pInterface);
    }
  }

  ~ezTestLogSystemScope()
  {
    if (m_bCatchMessagesGlobally)
    {
      ezGlobalLog::SetGlobalLogOverride(nullptr);
    }
  }

private:
  bool m_bCatchMessagesGlobally = false;
};
