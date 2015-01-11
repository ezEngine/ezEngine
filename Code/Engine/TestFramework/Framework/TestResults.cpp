#include <TestFramework/PCH.h>
#include <TestFramework/Framework/TestResults.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Time/Timestamp.h>

////////////////////////////////////////////////////////////////////////
// ezTestOutput public functions
////////////////////////////////////////////////////////////////////////

const char* const ezTestOutput::s_Names[] = {"StartOutput", "BeginBlock", "EndBlock", "ImportantInfo",
                                          "Details", "Success", "Message", "Error", "Duration", "FinalResult"};

const char* ezTestOutput::ToString(Enum type)
{
  return s_Names[type];
}

ezTestOutput::Enum ezTestOutput::FromString(const char* szName)
{
  for (ezUInt32 i = 0; i < AllOutputTypes; ++i)
  {
    if (strcmp(szName, s_Names[i]) == 0)
      return (ezTestOutput::Enum)i;
  }
  return InvalidType;
}


////////////////////////////////////////////////////////////////////////
// ezTestResultData public functions
////////////////////////////////////////////////////////////////////////

void ezTestResultData::Reset()
{
  m_bExecuted = false;
  m_bSuccess = false;
  m_iTestAsserts = 0;
  m_fTestDuration = 0.0;
  m_iFirstOutput = -1;
  m_iLastOutput = -1;
}

void ezTestResultData::AddOutput(ezInt32 iOutputIndex)
{
  if (m_iFirstOutput == -1)
  {
    m_iFirstOutput = iOutputIndex;
    m_iLastOutput = iOutputIndex;
  }
  else
  {
    m_iLastOutput = iOutputIndex;
  }
}


////////////////////////////////////////////////////////////////////////
// ezTestResultData public functions
////////////////////////////////////////////////////////////////////////

ezTestConfiguration::ezTestConfiguration()
  : m_uiInstalledMainMemory(0), m_uiMemoryPageSize(0), m_uiCPUCoreCount(0), m_b64BitOS(false), m_b64BitApplication(false), m_iDateTime(0), m_iRCSRevision(-1)
{
}


////////////////////////////////////////////////////////////////////////
// ezTestFrameworkResult public functions
////////////////////////////////////////////////////////////////////////

void ezTestFrameworkResult::Clear()
{
  m_Tests.clear();
  m_Errors.clear();
  m_TestOutput.clear();
}

void ezTestFrameworkResult::SetupTests(const std::deque<ezTestEntry>& tests, const ezTestConfiguration& config)
{
  m_config = config;
  Clear();

  const ezUInt32 uiTestCount = (ezUInt32)tests.size();
  for (ezUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    m_Tests.push_back(ezTestResult(tests[uiTestIdx].m_szTestName));

    const ezUInt32 uiSubTestCount = (ezUInt32)tests[uiTestIdx].m_SubTests.size();
    for (ezUInt32 uiSubTestIdx = 0; uiSubTestIdx < uiSubTestCount; ++uiSubTestIdx)
    {
      m_Tests[uiTestIdx].m_SubTests.push_back(ezSubTestResult(tests[uiTestIdx].m_SubTests[uiSubTestIdx].m_szSubTestName));
    }
  }
}

void::ezTestFrameworkResult::Reset()
{
  const ezUInt32 uiTestCount = (ezUInt32)m_Tests.size();
  for (ezUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    m_Tests[uiTestIdx].Reset();
  }
  m_Errors.clear();
  m_TestOutput.clear();
}

bool ezTestFrameworkResult::WriteJsonToFile(const char* szAbsFileName) const
{
  ezStartup::StartupCore();
  // Make sure we can access absolute file paths
  ezFileSystem::RegisterDataDirectoryFactory(ezDataDirectory::FolderType::Factory);
  ezFileSystem::AddDataDirectory("");

  {
    ezFileWriter file;
    if (file.Open(szAbsFileName) == EZ_FAILURE)
    {
      ezStartup::ShutdownCore();
      return false;
    }
    ezStandardJSONWriter js;
    js.SetOutputStream(&file);

    js.BeginObject();
    {
      js.BeginObject("configuration");
      {
        js.AddVariableUInt64("m_uiInstalledMainMemory", m_config.m_uiInstalledMainMemory);
        js.AddVariableUInt32("m_uiMemoryPageSize", m_config.m_uiMemoryPageSize);
        js.AddVariableUInt32("m_uiCPUCoreCount", m_config.m_uiCPUCoreCount);
        js.AddVariableBool("m_b64BitOS", m_config.m_b64BitOS);
        js.AddVariableBool("m_b64BitApplication", m_config.m_b64BitApplication);
        js.AddVariableString("m_sPlatformName", m_config.m_sPlatformName.c_str());
        js.AddVariableString("m_sBuildConfiguration", m_config.m_sBuildConfiguration.c_str());
        js.AddVariableInt64("m_iDateTime", m_config.m_iDateTime);
        js.AddVariableInt32("m_iRCSRevision", m_config.m_iRCSRevision);
        js.AddVariableString("m_sHostName", m_config.m_sHostName.c_str());
      }
      js.EndObject();

      // Output Messages
      js.BeginArray("messages");
      {
        ezUInt32 uiMessages = GetOutputMessageCount();
        for (ezUInt32 uiMessageIdx = 0; uiMessageIdx < uiMessages; ++uiMessageIdx)
        {
          const ezTestOutputMessage* pMessage = GetOutputMessage(uiMessageIdx);
          js.BeginObject();
          {
            js.AddVariableString("m_Type", ezTestOutput::ToString(pMessage->m_Type));
            js.AddVariableString("m_sMessage", pMessage->m_sMessage.c_str());
            if (pMessage->m_iErrorIndex != -1)
              js.AddVariableInt32("m_iErrorIndex", pMessage->m_iErrorIndex);
          }
          js.EndObject();
        }
      }
      js.EndArray();

      // Error Messages
      js.BeginArray("errors");
      {
        ezUInt32 uiMessages = GetErrorMessageCount();
        for (ezUInt32 uiMessageIdx = 0; uiMessageIdx < uiMessages; ++uiMessageIdx)
        {
          const ezTestErrorMessage* pMessage = GetErrorMessage(uiMessageIdx);
          js.BeginObject();
          {
            js.AddVariableString("m_sError", pMessage->m_sError.c_str());
            js.AddVariableString("m_sBlock", pMessage->m_sBlock.c_str());
            js.AddVariableString("m_sFile", pMessage->m_sFile.c_str());
            js.AddVariableString("m_sFunction", pMessage->m_sFunction.c_str());
            js.AddVariableInt32("m_iLine", pMessage->m_iLine);
            js.AddVariableString("m_sMessage", pMessage->m_sMessage.c_str());
          }
          js.EndObject();
        }
      }
      js.EndArray();

      // Tests
      js.BeginArray("tests");
      {
        ezUInt32 uiTests = GetTestCount();
        for (ezUInt32 uiTestIdx = 0; uiTestIdx < uiTests; ++uiTestIdx)
        {
          const ezTestResultData& testResult = GetTestResultData(uiTestIdx, -1);       
          js.BeginObject();
          {
            js.AddVariableString("m_sName", testResult.m_sName.c_str());
            js.AddVariableBool("m_bExecuted", testResult.m_bExecuted);
            js.AddVariableBool("m_bSuccess", testResult.m_bSuccess);
            js.AddVariableInt32("m_iTestAsserts", testResult.m_iTestAsserts);
            js.AddVariableDouble("m_fTestDuration", testResult.m_fTestDuration);
            js.AddVariableInt32("m_iFirstOutput", testResult.m_iFirstOutput);
            js.AddVariableInt32("m_iLastOutput", testResult.m_iLastOutput);

            // Sub Tests
            js.BeginArray("subTests");
            {
              ezUInt32 uiSubTests = GetSubTestCount(uiTestIdx);
              for (ezUInt32 uiSubTestIdx = 0; uiSubTestIdx < uiSubTests; ++uiSubTestIdx)
              {
                const ezTestResultData& subTestResult = GetTestResultData(uiTestIdx, uiSubTestIdx);  
                js.BeginObject();
                {
                  js.AddVariableString("m_sName", subTestResult.m_sName.c_str());
                  js.AddVariableBool("m_bExecuted", subTestResult.m_bExecuted);
                  js.AddVariableBool("m_bSuccess", subTestResult.m_bSuccess);
                  js.AddVariableInt32("m_iTestAsserts", subTestResult.m_iTestAsserts);
                  js.AddVariableDouble("m_fTestDuration", subTestResult.m_fTestDuration);
                  js.AddVariableInt32("m_iFirstOutput", subTestResult.m_iFirstOutput);
                  js.AddVariableInt32("m_iLastOutput", subTestResult.m_iLastOutput);
                }
                js.EndObject();
              }
            }
            js.EndArray(); // subTests
          }      
          js.EndObject();
        }
      }
      js.EndArray(); // tests

    }
    js.EndObject();
  }

  ezStartup::ShutdownCore();
  return true;
}

ezUInt32 ezTestFrameworkResult::GetTestCount(ezTestResultQuery::Enum countQuery) const
{
  ezUInt32 uiAccumulator = 0;
  const ezUInt32 uiTests = (ezUInt32)m_Tests.size();

  if (countQuery == ezTestResultQuery::Count)
    return uiTests;

  if (countQuery == ezTestResultQuery::Errors)
    return (ezUInt32)m_Errors.size();

  for (ezUInt32 uiTest = 0; uiTest < uiTests; ++uiTest)
  {
    switch (countQuery)
    {
    case ezTestResultQuery::Executed:
      uiAccumulator += m_Tests[uiTest].m_Result.m_bExecuted ? 1 : 0;
      break;
    case ezTestResultQuery::Success:
      uiAccumulator += m_Tests[uiTest].m_Result.m_bSuccess ? 1 : 0;
      break;
    default:
      break;
    }
  }
  return uiAccumulator;
}

ezUInt32 ezTestFrameworkResult::GetSubTestCount(ezUInt32 uiTestIndex, ezTestResultQuery::Enum countQuery) const
{
  if (uiTestIndex >= (ezUInt32)m_Tests.size())
    return 0;

  const ezTestResult& test = m_Tests[uiTestIndex];
  ezUInt32 uiAccumulator = 0;
  const ezUInt32 uiSubTests = (ezUInt32)test.m_SubTests.size();

  if (countQuery == ezTestResultQuery::Count)
    return uiSubTests;

  if (countQuery == ezTestResultQuery::Errors)
  {
    for (ezInt32 iOutputIdx = test.m_Result.m_iFirstOutput; iOutputIdx <= test.m_Result.m_iLastOutput && iOutputIdx != -1; ++iOutputIdx)
    {
      if (m_TestOutput[iOutputIdx].m_Type == ezTestOutput::Error)
        uiAccumulator++;
    }
    return uiAccumulator;
  }

  for (ezUInt32 uiSubTest = 0; uiSubTest < uiSubTests; ++uiSubTest)
  {
    switch (countQuery)
    {
    case ezTestResultQuery::Executed:
      uiAccumulator += test.m_SubTests[uiSubTest].m_Result.m_bExecuted ? 1 : 0;
      break;
    case ezTestResultQuery::Success:
      uiAccumulator += test.m_SubTests[uiSubTest].m_Result.m_bSuccess ? 1 : 0;
      break;
    default:
      break;
    }
  }
  return uiAccumulator;
}

ezInt32 ezTestFrameworkResult::GetTestIndexByName(const char* szTestName) const
{
  ezInt32 iTestCount = (ezInt32)GetTestCount();
  for (ezInt32 i = 0; i < iTestCount; ++i)
  {
    if (m_Tests[i].m_Result.m_sName.compare(szTestName) == 0)
      return i;
  }
  return -1;
}

ezInt32 ezTestFrameworkResult::GetSubTestIndexByName(ezUInt32 uiTestIndex, const char* szSubTestName) const
{
  if (uiTestIndex >= GetTestCount())
    return -1;

  ezInt32 iSubTestCount = (ezInt32)GetSubTestCount(uiTestIndex);
  for (ezInt32 i = 0; i < iSubTestCount; ++i)
  {
    if (m_Tests[uiTestIndex].m_SubTests[i].m_Result.m_sName.compare(szSubTestName) == 0)
      return i;
  }
  return -1;
}

double ezTestFrameworkResult::GetTotalTestDuration() const
{
  double fTotalTestDuration = 0.0;
  const ezUInt32 uiTests = (ezUInt32)m_Tests.size();
  for (ezUInt32 uiTest = 0; uiTest < uiTests; ++uiTest)
  {
    fTotalTestDuration += m_Tests[uiTest].m_Result.m_fTestDuration;
  }
  return fTotalTestDuration;
}

const ezTestResultData& ezTestFrameworkResult::GetTestResultData(ezUInt32 uiTestIndex, ezInt32 iSubTestIndex) const
{
  return (iSubTestIndex == -1) ? m_Tests[uiTestIndex].m_Result : m_Tests[uiTestIndex].m_SubTests[iSubTestIndex].m_Result;
}

void ezTestFrameworkResult::TestOutput(ezUInt32 uiTestIndex, ezInt32 iSubTestIndex, ezTestOutput::Enum Type, const char* szMsg)
{
  if (uiTestIndex != -1)
  {
    m_Tests[uiTestIndex].m_Result.AddOutput((ezInt32)m_TestOutput.size());
  }

  if (iSubTestIndex != -1)
  {
    m_Tests[uiTestIndex].m_SubTests[iSubTestIndex].m_Result.AddOutput((ezInt32)m_TestOutput.size());
  }

  m_TestOutput.push_back(ezTestOutputMessage());
  ezTestOutputMessage& outputMessage = *m_TestOutput.rbegin();
  outputMessage.m_Type = Type;
  outputMessage.m_sMessage.assign(szMsg);
}

void ezTestFrameworkResult::TestError(ezUInt32 uiTestIndex, ezInt32 iSubTestIndex,
                                        const char* szError, const char* szBlock, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg)
{
  // In case there is no message set, we use the error as the message.
  TestOutput(uiTestIndex, iSubTestIndex, ezTestOutput::Error, szError);
  m_TestOutput.rbegin()->m_iErrorIndex = (ezInt32)m_Errors.size();

  m_Errors.push_back(ezTestErrorMessage());
  ezTestErrorMessage& errorMessage = *m_Errors.rbegin();
  errorMessage.m_sError.assign(szError);
  errorMessage.m_sBlock.assign(szBlock);
  errorMessage.m_sFile.assign(szFile);
  errorMessage.m_iLine = iLine;
  errorMessage.m_sFunction.assign(szFunction);
  errorMessage.m_sMessage.assign(szMsg);
}

void ezTestFrameworkResult::TestResult(ezUInt32 uiTestIndex, ezInt32 iSubTestIndex, bool bSuccess, double fDuration)
{
  ezTestResultData& Result = (iSubTestIndex == -1) ? m_Tests[uiTestIndex].m_Result : m_Tests[uiTestIndex].m_SubTests[iSubTestIndex].m_Result;

  Result.m_bExecuted = true;
  Result.m_bSuccess = bSuccess;
  Result.m_fTestDuration = fDuration;

  // Accumulate sub-test duration onto test duration to get duration feedback while the sub-tests are running.
  // Final time will be set again once the entire test finishes and currently these times are identical as
  // init and de-init times aren't measured at the moment due to missing timer when engine is shut down.
  if (iSubTestIndex != -1)
  {
    m_Tests[uiTestIndex].m_Result.m_fTestDuration += fDuration;
  }
}

void ezTestFrameworkResult::AddAsserts(ezUInt32 uiTestIndex, ezInt32 iSubTestIndex, int iCount)
{
  if (uiTestIndex != -1)
  {
    m_Tests[uiTestIndex].m_Result.m_iTestAsserts += iCount;
  }

  if (iSubTestIndex != -1)
  {
    m_Tests[uiTestIndex].m_SubTests[iSubTestIndex].m_Result.m_iTestAsserts += iCount;
  }
}

ezUInt32 ezTestFrameworkResult::GetOutputMessageCount(ezInt32 iTestIndex, ezInt32 iSubTestIndex, ezTestOutput::Enum Type) const
{
  if (iTestIndex == -1 && Type == ezTestOutput::AllOutputTypes)
    return (ezUInt32)m_TestOutput.size();

  ezInt32 iStartIdx = 0;
  ezInt32 iEndIdx = (ezInt32)m_TestOutput.size() - 1;

  if (iTestIndex != -1)
  {
    const ezTestResultData& result = GetTestResultData(iTestIndex, iSubTestIndex);
    iStartIdx = result.m_iFirstOutput;
    iEndIdx = result.m_iLastOutput;

    // If no messages have been output (yet) for the given test we early-out here.
    if (iStartIdx == -1)
      return 0;

    // If all message types should be counted we can simply return the range.
    if (Type == ezTestOutput::AllOutputTypes)
      return iEndIdx - iStartIdx + 1;
  }

  ezUInt32 uiAccumulator = 0;
  for (ezInt32 uiOutputMessageIdx = iStartIdx; uiOutputMessageIdx <= iEndIdx; ++uiOutputMessageIdx)
  {
    if (m_TestOutput[uiOutputMessageIdx].m_Type == Type)
      uiAccumulator++;
  }
  return uiAccumulator;
}

const ezTestOutputMessage* ezTestFrameworkResult::GetOutputMessage(ezUInt32 uiOutputMessageIdx) const
{
  return &m_TestOutput[uiOutputMessageIdx];
}

ezUInt32 ezTestFrameworkResult::GetErrorMessageCount(ezInt32 iTestIndex, ezInt32 iSubTestIndex) const
{
  // If no test is given we can simply return the total error count.
  if (iTestIndex == -1)
  {
    return (ezUInt32)m_Errors.size();
  }

  return GetOutputMessageCount(iTestIndex, iSubTestIndex, ezTestOutput::Error);
}

const ezTestErrorMessage* ezTestFrameworkResult::GetErrorMessage(ezUInt32 uiErrorMessageIdx) const
{
  return &m_Errors[uiErrorMessageIdx];
}


////////////////////////////////////////////////////////////////////////
// ezTestFrameworkResult public functions
////////////////////////////////////////////////////////////////////////

void ezTestFrameworkResult::ezTestResult::Reset()
{
  m_Result.Reset();
  const ezUInt32 uiSubTestCount = (ezUInt32)m_SubTests.size();
  for (ezUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
  {
    m_SubTests[uiSubTest].m_Result.Reset();
  }
}



EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_TestResults);

