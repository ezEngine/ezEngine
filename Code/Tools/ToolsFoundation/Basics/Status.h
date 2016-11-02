#pragma once

/// \file

#include <ToolsFoundation/Basics.h>
#include <Foundation/Strings/String.h>

class ezLogInterface;

/// \brief An ezResult with an additional message for the reason of failure
struct EZ_TOOLSFOUNDATION_DLL ezStatus
{
  explicit ezStatus() : m_Result(EZ_FAILURE)
  {
  }

  explicit ezStatus(ezResult res, ezStringView sError) : m_Result(res), m_sMessage(sError)
  {
  }

  explicit ezStatus(ezResult r) : m_Result(r)
  {
  }

  explicit ezStatus(const char* szError, ...);

  EZ_FORCE_INLINE bool Succeeded() const { return m_Result.Succeeded(); }
  EZ_FORCE_INLINE bool Failed() const { return m_Result.Failed(); }
  void LogFailure(ezLogInterface* pLog = nullptr);

  ezResult m_Result;
  ezString m_sMessage;
};


