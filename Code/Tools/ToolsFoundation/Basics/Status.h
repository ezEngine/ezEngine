#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Strings/String.h>

struct ezStatus
{
  ezStatus() : m_Result(EZ_FAILURE)
  {
  }

  ezResult m_Result;
  ezString m_sError;
};

