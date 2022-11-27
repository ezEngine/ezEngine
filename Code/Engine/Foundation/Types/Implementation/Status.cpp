#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Status.h>

void ezResult::AssertSuccess(const char* msg /*= nullptr*/, const char* details /*= nullptr*/) const
{
  if (Succeeded())
    return;

  if (msg)
  {
    EZ_REPORT_FAILURE(msg, details);
  }
  else
  {
    EZ_REPORT_FAILURE("An operation failed unexpectedly.");
  }
}

ezStatus::ezStatus(const ezFormatString& fmt)
  : m_Result(EZ_FAILURE)
{
  ezStringBuilder sMsg;
  m_sMessage = fmt.GetText(sMsg);
}

void ezStatus::LogFailure(ezLogInterface* pLog)
{
  if (Failed())
  {
    ezLogInterface* pInterface = pLog ? pLog : ezLog::GetThreadLocalLogSystem();
    ezLog::Error(pInterface, "{0}", m_sMessage);
  }
}


EZ_STATICLINK_FILE(Foundation, Foundation_Types_Implementation_Status);
