#include <TestFrameworkPCH.h>

#include <TestFramework/Utilities/TestLogInterface.h>

#include <TestFramework/Framework/TestFramework.h>

 ezTestLogInterface::~ezTestLogInterface()
{
  for (const auto& countMsgPair : m_expectedErrors)
  {
    ezInt32 count = countMsgPair.first;
    EZ_TEST_BOOL_MSG(count == 0, "Error message \"%s\" was logged %d times %s than expected.", countMsgPair.second.GetData(),
            count < 0 ? -count : count, count < 0 ? "more" : "less");
  }
}

void ezTestLogInterface::HandleLogMessage(const ezLoggingEventData& le)
{
  for (auto& countMsgPair : m_expectedErrors)
  {
    if (ezStringUtils::FindSubString(le.m_szText, countMsgPair.second))
    {
      --countMsgPair.first;
      return;
    }
  }

  if (m_pParentLog)
  {
    m_pParentLog->HandleLogMessage(le);
  }
}

void ezTestLogInterface::ExpectError(const char* msg, const ezInt32 count)
{
  // Do not allow initial count to be less than 1, but use signed int to keep track
  // of error messages that were encountered more often than expected.
  EZ_ASSERT_DEV(count >= 1, "Error message needs to be expected at least once");
  m_expectedErrors.PushBack(std::make_pair(count, msg));
}
