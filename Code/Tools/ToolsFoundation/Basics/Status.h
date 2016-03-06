#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

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

  explicit ezStatus(const char* szError, ...) : m_Result(EZ_FAILURE)
  {
    va_list args;
    va_start(args, szError);

    ezStringBuilder sMsg;
    sMsg.FormatArgs(szError, args);

    va_end(args);

    m_sMessage = sMsg;
  }

  ezResult m_Result;
  ezString m_sMessage;
};

