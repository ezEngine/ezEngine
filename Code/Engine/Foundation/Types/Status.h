#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Strings/StringBuilder.h>

class ezLogInterface;

/// \brief An ezResult with an additional message for the reason of failure
struct [[nodiscard]] EZ_FOUNDATION_DLL ezStatus
{
  /// \brief Sets the status to EZ_FAILURE and stores the error message.
  explicit ezStatus(const char* szError)
    : m_Result(EZ_FAILURE)
    , m_sMessage(szError)
  {
  }

  /// \brief Sets the status to EZ_FAILURE and stores the error message.
  explicit ezStatus(ezStringView sError)
    : m_Result(EZ_FAILURE)
    , m_sMessage(sError)
  {
  }

  /// \brief Sets the status, but doesn't store a message string.
  EZ_ALWAYS_INLINE ezStatus(ezResult r)
    : m_Result(r)
  {
  }

  /// \brief Sets the status, but doesn't store a message string.
  EZ_ALWAYS_INLINE ezStatus(ezResultEnum r)
    : m_Result(r)
  {
  }

  /// \brief Sets the status to EZ_FAILURE and stores the error message. Can be used with ezFmt().
  explicit ezStatus(const ezFormatString& fmt);

  [[nodiscard]] ezResult GetResult() const { return m_Result; }

  [[nodiscard]] EZ_ALWAYS_INLINE bool Succeeded() const { return m_Result.Succeeded(); }
  [[nodiscard]] EZ_ALWAYS_INLINE bool Failed() const { return m_Result.Failed(); }

  /// \brief Used to silence compiler warnings, when success or failure doesn't matter.
  EZ_ALWAYS_INLINE void IgnoreResult()
  {
    /* dummy to be called when a return value is [[nodiscard]] but the result is not needed */
  }

  /// \brief If the state is EZ_FAILURE, the message is written to the given log (or the currently active thread-local log).
  ///
  /// The return value is the same as 'Failed()' but isn't marked as [[nodiscard]], ie returns true, if a failure happened,
  /// so can be used in a conditional.
  bool LogFailure(ezLogInterface* pLog = nullptr) const;

  /// \brief Asserts that the function succeeded. In case of failure, the program will terminate.
  ///
  /// If \a szMsg is given, this will be the assert message.
  /// Additionally m_sMessage will be included as a detailed message.
  void AssertSuccess(const char* szMsg = nullptr) const;

  [[nodiscard]] const ezString& GetMessageString() const { return m_sMessage; }

private:
  ezResult m_Result;
  ezString m_sMessage;
};

EZ_ALWAYS_INLINE ezResult ezToResult(const ezStatus& result)
{
  return result.GetResult();
}
