#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Strings/String.h>

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

  ezStatus(const char* szError) : m_Result(EZ_FAILURE)
  {
    m_sError = szError;
  }

  ezResult m_Result;
  ezString m_sError;
};

