#include <TestFramework/TestFrameworkPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <TestFramework/Framework/TestResults.h>

////////////////////////////////////////////////////////////////////////
// ezTestOutput public functions
////////////////////////////////////////////////////////////////////////

const char* const ezTestOutput::s_Names[] = {
  "StartOutput", "BeginBlock", "EndBlock", "ImportantInfo", "Details", "Success", "Message", "Warning", "Error", "Duration", "FinalResult"};

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
  m_sCustomStatus.clear();
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

  = default;


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
  m_Config = config;
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

void ::ezTestFrameworkResult::Reset()
{
  const ezUInt32 uiTestCount = (ezUInt32)m_Tests.size();
  for (ezUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    m_Tests[uiTestIdx].Reset();
  }
  m_Errors.clear();
  m_TestOutput.clear();
}

bool ezTestFrameworkResult::WriteJsonToFile(const char* szFileName) const
{
  ezStartup::StartupCoreSystems();
  EZ_SCOPE_EXIT(ezStartup::ShutdownCoreSystems());

  {
    ezStringBuilder jsonFilename;
    if (ezPathUtils::IsAbsolutePath(szFileName))
    {
      // Make sure we can access raw absolute file paths
      if (ezFileSystem::AddDataDirectory("", "jsonoutput", ":", ezDataDirUsage::AllowWrites).Failed())
        return false;

      jsonFilename = szFileName;
    }
    else
    {
      // If this is a relative path, we use the eztest/ data directory to make sure that this works properly with the fileserver.
      if (ezFileSystem::AddDataDirectory(">eztest/", "jsonoutput", ":", ezDataDirUsage::AllowWrites).Failed())
        return false;

      jsonFilename = ":";
      jsonFilename.AppendPath(szFileName);
    }

    ezFileWriter file;
    if (file.Open(jsonFilename).Failed())
    {
      return false;
    }
    ezStandardJSONWriter js;
    js.SetOutputStream(&file);

    js.BeginObject();
    {
      js.BeginObject("configuration");
      {
        js.AddVariableUInt64("m_uiInstalledMainMemory", m_Config.m_uiInstalledMainMemory);
        js.AddVariableUInt32("m_uiMemoryPageSize", m_Config.m_uiMemoryPageSize);
        js.AddVariableUInt32("m_uiCPUCoreCount", m_Config.m_uiCPUCoreCount);
        js.AddVariableBool("m_b64BitOS", m_Config.m_b64BitOS);
        js.AddVariableBool("m_b64BitApplication", m_Config.m_b64BitApplication);
        js.AddVariableString("m_sPlatformName", m_Config.m_sPlatformName.c_str());
        js.AddVariableString("m_sBuildConfiguration", m_Config.m_sBuildConfiguration.c_str());
        js.AddVariableInt64("m_iDateTime", m_Config.m_iDateTime);
        js.AddVariableInt32("m_iRCSRevision", m_Config.m_iRCSRevision);
        js.AddVariableString("m_sHostName", m_Config.m_sHostName.c_str());
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
          const ezTestResultData& testResult = GetTestResultData(uiTestIdx, ezInvalidIndex);
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

ezUInt32 ezTestFrameworkResult::GetTestIndexByName(const char* szTestName) const
{
  const ezUInt32 uiTestCount = GetTestCount();
  for (ezUInt32 i = 0; i < uiTestCount; ++i)
  {
    if (m_Tests[i].m_Result.m_sName.compare(szTestName) == 0)
      return i;
  }

  return ezInvalidIndex;
}

ezUInt32 ezTestFrameworkResult::GetSubTestIndexByName(ezUInt32 uiTestIndex, const char* szSubTestName) const
{
  if (uiTestIndex >= GetTestCount())
    return ezInvalidIndex;

  const ezUInt32 uiSubTestCount = GetSubTestCount(uiTestIndex);
  for (ezUInt32 i = 0; i < uiSubTestCount; ++i)
  {
    if (m_Tests[uiTestIndex].m_SubTests[i].m_Result.m_sName.compare(szSubTestName) == 0)
      return i;
  }

  return ezInvalidIndex;
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

const ezTestResultData& ezTestFrameworkResult::GetTestResultData(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex) const
{
  return (uiSubTestIndex == ezInvalidIndex) ? m_Tests[uiTestIndex].m_Result : m_Tests[uiTestIndex].m_SubTests[uiSubTestIndex].m_Result;
}

void ezTestFrameworkResult::TestOutput(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex, ezTestOutput::Enum type, const char* szMsg)
{
  if (uiTestIndex != ezInvalidIndex)
  {
    m_Tests[uiTestIndex].m_Result.AddOutput((ezUInt32)m_TestOutput.size());
    if (uiSubTestIndex != ezInvalidIndex)
    {
      m_Tests[uiTestIndex].m_SubTests[uiSubTestIndex].m_Result.AddOutput((ezUInt32)m_TestOutput.size());
    }
  }

  m_TestOutput.push_back(ezTestOutputMessage());
  ezTestOutputMessage& outputMessage = *m_TestOutput.rbegin();
  outputMessage.m_Type = type;
  outputMessage.m_sMessage.assign(szMsg);
}

void ezTestFrameworkResult::TestError(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex, const char* szError, const char* szBlock, const char* szFile,
  ezInt32 iLine, const char* szFunction, const char* szMsg)
{
  // In case there is no message set, we use the error as the message.
  TestOutput(uiTestIndex, uiSubTestIndex, ezTestOutput::Error, szError);
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

void ezTestFrameworkResult::TestResult(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex, bool bSuccess, double fDuration)
{
  ezTestResultData& Result = (uiSubTestIndex == ezInvalidIndex) ? m_Tests[uiTestIndex].m_Result : m_Tests[uiTestIndex].m_SubTests[uiSubTestIndex].m_Result;

  Result.m_bExecuted = true;
  Result.m_bSuccess = bSuccess;
  Result.m_fTestDuration = fDuration;

  // Accumulate sub-test duration onto test duration to get duration feedback while the sub-tests are running.
  // Final time will be set again once the entire test finishes and currently these times are identical as
  // init and de-init times aren't measured at the moment due to missing timer when engine is shut down.
  if (uiSubTestIndex != ezInvalidIndex)
  {
    m_Tests[uiTestIndex].m_Result.m_fTestDuration += fDuration;
  }
}

void ezTestFrameworkResult::AddAsserts(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex, int iCount)
{
  if (uiTestIndex != ezInvalidIndex)
  {
    m_Tests[uiTestIndex].m_Result.m_iTestAsserts += iCount;
  }

  if (uiSubTestIndex != ezInvalidIndex)
  {
    m_Tests[uiTestIndex].m_SubTests[uiSubTestIndex].m_Result.m_iTestAsserts += iCount;
  }
}

void ezTestFrameworkResult::SetCustomStatus(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex, const char* szCustomStatus)
{
  if (uiTestIndex != ezInvalidIndex && uiSubTestIndex != ezInvalidIndex)
  {
    m_Tests[uiTestIndex].m_SubTests[uiSubTestIndex].m_Result.m_sCustomStatus = szCustomStatus;
  }
}

ezUInt32 ezTestFrameworkResult::GetOutputMessageCount(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex, ezTestOutput::Enum type) const
{
  if (uiTestIndex == ezInvalidIndex && type == ezTestOutput::AllOutputTypes)
    return (ezUInt32)m_TestOutput.size();

  ezInt32 iStartIdx = 0;
  ezInt32 iEndIdx = (ezInt32)m_TestOutput.size() - 1;

  if (uiTestIndex != ezInvalidIndex)
  {
    const ezTestResultData& result = GetTestResultData(uiTestIndex, uiSubTestIndex);
    iStartIdx = result.m_iFirstOutput;
    iEndIdx = result.m_iLastOutput;

    // If no messages have been output (yet) for the given test we early-out here.
    if (iStartIdx == -1)
      return 0;

    // If all message types should be counted we can simply return the range.
    if (type == ezTestOutput::AllOutputTypes)
      return iEndIdx - iStartIdx + 1;
  }

  ezUInt32 uiAccumulator = 0;
  for (ezInt32 uiOutputMessageIdx = iStartIdx; uiOutputMessageIdx <= iEndIdx; ++uiOutputMessageIdx)
  {
    if (m_TestOutput[uiOutputMessageIdx].m_Type == type)
      uiAccumulator++;
  }
  return uiAccumulator;
}

const ezTestOutputMessage* ezTestFrameworkResult::GetOutputMessage(ezUInt32 uiOutputMessageIdx) const
{
  return &m_TestOutput[uiOutputMessageIdx];
}

ezUInt32 ezTestFrameworkResult::GetErrorMessageCount(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex) const
{
  // If no test is given we can simply return the total error count.
  if (uiTestIndex == ezInvalidIndex)
  {
    return (ezUInt32)m_Errors.size();
  }

  return GetOutputMessageCount(uiTestIndex, uiSubTestIndex, ezTestOutput::Error);
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
