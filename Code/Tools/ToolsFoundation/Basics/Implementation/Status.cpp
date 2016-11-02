#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Basics/Status.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Logging/Log.h>

ezStatus::ezStatus(const char* szError, ...) : m_Result(EZ_FAILURE)
{
  va_list args;
  va_start(args, szError);

  ezStringBuilder sMsg;
  sMsg.FormatArgs(szError, args);

  va_end(args);

  m_sMessage = sMsg;
}

void ezStatus::LogFailure(ezLogInterface* pLog)
{
  if (Failed())
  {
    ezLogInterface* pInterface = pLog ? pLog : ezLog::GetDefaultLogSystem();
    ezLog::Error(pInterface, "%s", m_sMessage.GetData());
  }
}
