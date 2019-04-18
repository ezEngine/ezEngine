#include <TestFrameworkPCH.h>

#include <TestFramework/Utilities/TestLogInterface.h>

#include <TestFramework/Framework/TestFramework.h>

 ezTestLogInterface::~ezTestLogInterface()
{
  for (const ExpectedMsg& msg : m_expectedMessages)
  {
    ezInt32 count = msg.m_iCount;
    EZ_TEST_BOOL_MSG(count == 0, "Message \"%s\" was logged %d times %s than expected.", msg.m_sMsgSubString.GetData(),
            count < 0 ? -count : count, count < 0 ? "more" : "less");
  }
}

void ezTestLogInterface::HandleLogMessage(const ezLoggingEventData& le)
{
  for (ExpectedMsg& msg : m_expectedMessages)
  {
    if (msg.m_Type != ezLogMsgType::All && le.m_EventType != msg.m_Type)
      continue;

    if (ezStringUtils::FindSubString(le.m_szText, msg.m_sMsgSubString))
    {
      --msg.m_iCount;

      // filter out error and warning messages entirely
      if (le.m_EventType >= ezLogMsgType::ErrorMsg && le.m_EventType <= ezLogMsgType::WarningMsg)
        return;

      // pass all other messages along to the parent log
      break;
    }
  }

  if (m_pParentLog)
  {
    m_pParentLog->HandleLogMessage(le);
  }
}

void ezTestLogInterface::ExpectMessage(const char* msg, ezLogMsgType::Enum type /*= ezLogMsgType::All*/, ezInt32 count /*= 1*/)
{
  // Do not allow initial count to be less than 1, but use signed int to keep track
  // of error messages that were encountered more often than expected.
  EZ_ASSERT_DEV(count >= 1, "Message needs to be expected at least once");

  ExpectedMsg& em = m_expectedMessages.ExpandAndGetRef();
  em.m_sMsgSubString = msg;
  em.m_iCount = count;
  em.m_Type = type;
}
