#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Strings/StringBuilder.h>

class ezLogInterface;

/// \brief An ezResult with an additional message for the reason of failure
struct EZ_FOUNDATION_DLL ezStatus
{
  EZ_ALWAYS_INLINE explicit ezStatus()
    : m_Result(EZ_FAILURE)
  {
  }

  // const char* version is needed for disambiguation
  explicit ezStatus(const char* szError)
    : m_Result(EZ_FAILURE)
    , m_sMessage(szError)
  {
  }

  explicit ezStatus(ezResult r, ezStringView sError)
    : m_Result(r)
    , m_sMessage(sError)
  {
  }

  explicit ezStatus(ezStringView sError)
    : m_Result(EZ_FAILURE)
    , m_sMessage(sError)
  {
  }

  EZ_ALWAYS_INLINE ezStatus(ezResult r)
    : m_Result(r)
  {
  }

  explicit ezStatus(const ezFormatString& fmt);

  [[nodiscard]] EZ_ALWAYS_INLINE bool Succeeded() const { return m_Result.Succeeded(); }
  [[nodiscard]] EZ_ALWAYS_INLINE bool Failed() const { return m_Result.Failed(); }

  /// \brief Same as 'Succeeded()'.
  ///
  /// Allows ezStatus to be used in if statements:
  ///  - if (r)
  ///  - if (!r)
  ///  - if (r1 && r2)
  ///  - if (r1 || r2)
  ///
  /// Disallows anything else implicitly, e.g. all these won't compile:
  ///   - if (r == true)
  ///   - bool b = r;
  ///   - void* p = r;
  ///   - return r; // with bool return type
  explicit operator bool() const { return m_Result.Succeeded(); }

  /// \brief Special case to prevent this from working: "bool b = !r"
  ezResult operator!() const { return ezResult(m_Result.Succeeded() ? EZ_FAILURE : EZ_SUCCESS); }

  /// \brief If the state is EZ_FAILURE, the message is written to the given log (or the currently active thread-local log).
  void LogFailure(ezLogInterface* pLog = nullptr);

  ezResult m_Result;
  ezString m_sMessage;
};

EZ_ALWAYS_INLINE ezResult ezToResult(const ezStatus& result)
{
  return result.m_Result;
}
