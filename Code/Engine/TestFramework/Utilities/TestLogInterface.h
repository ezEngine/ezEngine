#pragma once
#include <TestFramework/TestFrameworkDLL.h>
#include <Foundation/Logging/Log.h>

/// \brief An ezLogInterface that expects and handles error messages during test runs. Can be
/// used to ensure that expected error messages are produced by the tested functionality.
/// Expected error messages are not passed on and do not cause tests to fail.
class EZ_TEST_DLL ezTestLogInterface : public ezLogInterface
{
public:
  ezTestLogInterface() = default;
  ~ezTestLogInterface();
  virtual void HandleLogMessage(const ezLoggingEventData& le) override;
  /// \brief Add expected error message. Will fail the test when expected error message is not
  /// encountered. Can take an optional count, if messages are expected multiple times
  void ExpectError(const char* msg, ezInt32 count = 1);

  /// \brief Set the log interface that unhandled messages are forwarded to.
  void SetParentLog(ezLogInterface* pInterface) { m_pParentLog = pInterface; }

private:
  ezLogInterface* m_pParentLog = nullptr;
  ezHybridArray<std::pair<ezInt32, ezString>, 8> m_expectedErrors;
};

/// \brief A class that sets a custom ezTestLogInterface as the thread local default log system,
/// and resets the previous system when it goes out of scope. The test version passes the previous
/// ezLogInterface on to the ezTestLogInterface to enable passing on unhandled messages.
class EZ_TEST_DLL ezTestLogSystemScope : public ezLogSystemScope
{
public:
  explicit ezTestLogSystemScope(ezTestLogInterface* pInterface)
      : ezLogSystemScope(pInterface)
  {
    pInterface->SetParentLog(m_pPrevious);
  }
};
