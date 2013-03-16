#include <TestFramework/PCH.h>
#include <TestFramework/Framework/TestFramework.h>

const char* ezTestFramework::s_szTestBlockName = "";
ezInt32 ezTestFramework::s_iErrorCount = 0;
ezInt32 ezTestFramework::s_iTestsFailed = 0;
ezInt32 ezTestFramework::s_iTestsPassed = 0;
bool ezTestFramework::s_bAssertOnTestFail = true;

std::deque<ezTestFramework::OutputHandler> ezTestFramework::s_OutputHandlers;

void ezTestFramework::RegisterOutputHandler(OutputHandler Handler) 
{ 
  // do not register a handler twice
  for (ezUInt32 i = 0; i < s_OutputHandlers.size(); ++i)
  {
    if (s_OutputHandlers[i] == Handler)
      return;
  }

  s_OutputHandlers.push_back(Handler); 
}

ezInt32 ezTestFramework::GetErrorCount()
{
  return s_iErrorCount;
}

void ezTestFramework::Output(ezTestOutput::Enum Type, const char* szMsg, ...)
{
  switch (Type)
  {
  case ezTestOutput::Error:
    ++s_iErrorCount;
    break;
  default:
    break;
  }

  // format the output text
  va_list args;
  va_start (args, szMsg);

  char szBuffer[1024 * 10];
  vsprintf (szBuffer, szMsg, args);
  va_end (args);

  // pass the output to all the registered output handlers, which will then write it to the console, file, etc.
  for (ezUInt32 i = 0; i < s_OutputHandlers.size(); ++i)
  {
    s_OutputHandlers[i](Type, szBuffer);
  }
}

void ezTestFramework::GatherAllTests(std::deque<ezTestEntry>& out_AllTests)
{
  out_AllTests.clear();

  s_iErrorCount = 0;
  s_iTestsFailed = 0;
  s_iTestsPassed = 0;

  // first let all simple tests register themselves
  {
    ezRegisterSimpleTestHelper* pHelper = ezRegisterSimpleTestHelper::GetFirstInstance();

    while (pHelper)
    {
      pHelper->RegisterTest();

      pHelper = pHelper->GetNextInstance();
    }
  }

  ezTestBaseClass* pTest = ezTestBaseClass::GetFirstInstance();

  while (pTest)
  {
    pTest->ClearSubTests();
    pTest->SetupSubTests();

    ezTestEntry e;
    e.m_pTest = pTest;
    e.m_szTestName = pTest->GetTestName();

    for (ezUInt32 i = 0; i < pTest->m_Entries.size(); ++i)
    {
      ezSubTestEntry st;
      st.m_szSubTestName = pTest->m_Entries[i].m_szName;
      st.m_iSubTestIdentifier = pTest->m_Entries[i].m_iIdentifier;
      
      e.m_SubTests.push_back(st);
    }

    out_AllTests.push_back(e);

    pTest = pTest->GetNextInstance();
  }
}

void ezTestFramework::ExecuteAllTests(std::deque<ezTestEntry>& TestsToExecute)
{
  s_iErrorCount = 0;
  s_iTestsFailed = 0;
  s_iTestsPassed = 0;

  for (ezUInt32 i = 0; i < TestsToExecute.size(); ++i)
  {
    if (!TestsToExecute[i].m_bEnableTest)
      continue;

    if (TestsToExecute[i].m_pTest->ExecuteTest(TestsToExecute[i].m_SubTests, TestsToExecute[i].m_fTotalDuration) == EZ_SUCCESS)
      s_iTestsPassed++;
    else
      s_iTestsFailed++;
  }
}

