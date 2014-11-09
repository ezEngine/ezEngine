#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Strings/String.h>

struct ezStatus
{
  ezStatus() : m_Result(EZ_FAILURE)
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

