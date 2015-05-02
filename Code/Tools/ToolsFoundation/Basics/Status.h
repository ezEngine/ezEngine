#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

struct ezStatus
{
  ezStatus() : m_Result(EZ_FAILURE)
  {
  }

  ezStatus(ezResult res, ezStringView sError) : m_Result(res), m_sError(sError)
  {
  }

  ezStatus(ezResult r) : m_Result(r)
  {
  }

  ezStatus(const char* szError, ...) : m_Result(EZ_FAILURE)
  {
    va_list args;
    va_start(args, szError);

    ezStringBuilder sMsg;
    sMsg.FormatArgs(szError, args);

    va_end(args);

    m_sError = sMsg;
  }

  ezResult m_Result;
  ezString m_sError;
};

