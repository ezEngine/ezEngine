#pragma once

/// \file

#include <ToolsFoundation/Basics.h>
#include <Foundation/Strings/StringBuilder.h>

class ezLogInterface;

/// \brief An ezResult with an additional message for the reason of failure
struct EZ_TOOLSFOUNDATION_DLL ezStatus
{
  explicit ezStatus() : m_Result(EZ_FAILURE)
  {
  }

  explicit ezStatus(const char* szError) : m_Result(EZ_FAILURE), m_sMessage(szError)
  {
  }

  explicit ezStatus(ezResult r, ezStringView sError) : m_Result(r), m_sMessage(sError)
  {
  }

  explicit ezStatus(ezStringView sError) : m_Result(EZ_FAILURE), m_sMessage(sError)
  {
  }

  explicit ezStatus(ezResult r) : m_Result(r)
  {
  }

  explicit ezStatus(const ezFormatString& fmt);

  EZ_FORCE_INLINE bool Succeeded() const { return m_Result.Succeeded(); }
  EZ_FORCE_INLINE bool Failed() const { return m_Result.Failed(); }
  void LogFailure(ezLogInterface* pLog = nullptr);

  ezResult m_Result;
  ezString m_sMessage;
};


