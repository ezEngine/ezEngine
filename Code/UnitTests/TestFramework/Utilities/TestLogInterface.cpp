#include <TestFramework/TestFrameworkPCH.h>

#include <TestFramework/Utilities/TestLogInterface.h>

#include <TestFramework/Framework/TestFramework.h>

ezTestLogInterface::~ezTestLogInterface()
{
  for (const ExpectedMsg& msg : m_ExpectedMessages)
  {
    ezInt32 count = msg.m_iCount;
    EZ_TEST_BOOL_MSG(count == 0, "Message \"%s\" was logged %d times %s than expected.", msg.m_sMsgSubString.GetData(), count < 0 ? -count : count,
      count < 0 ? "more" : "less");
  }
}

void ezTestLogInterface::HandleLogMessage(const ezLoggingEventData& le)
{
  {
    // in case this interface is used with ezTestLogSystemScope to override the ezGlobalLog (see ezGlobalLog::SetGlobalLogOverride)
    // it must be thread-safe
    EZ_LOCK(m_Mutex);

    for (ExpectedMsg& msg : m_ExpectedMessages)
    {
      if (msg.m_Type != ezLogMsgType::All && le.m_EventType != msg.m_Type)
        continue;

      if (le.m_sText.FindSubString(msg.m_sMsgSubString))
      {
        --msg.m_iCount;

        // filter out error and warning messages entirely
        if (le.m_EventType >= ezLogMsgType::ErrorMsg && le.m_EventType <= ezLogMsgType::WarningMsg)
          return;

        // pass all other messages along to the parent log
        break;
      }
    }
  }

  if (m_pParentLog)
  {
    m_pParentLog->HandleLogMessage(le);
  }
}

void ezTestLogInterface::ExpectMessage(const char* szMsg, ezLogMsgType::Enum type /*= ezLogMsgType::All*/, ezInt32 iCount /*= 1*/)
{
  EZ_LOCK(m_Mutex);

  // Do not allow initial count to be less than 1, but use signed int to keep track
  // of error messages that were encountered more often than expected.
  EZ_ASSERT_DEV(iCount >= 1, "Message needs to be expected at least once");

  ExpectedMsg& em = m_ExpectedMessages.ExpandAndGetRef();
  em.m_sMsgSubString = szMsg;
  em.m_iCount = iCount;
  em.m_Type = type;
}
