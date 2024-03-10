#include <TestFramework/TestFrameworkPCH.h>

#include <TestFramework/Utilities/TestOrder.h>

/// Operator to sort tests alphabetically
inline bool SortTest_Operator(const ezTestEntry& lhs, const ezTestEntry& rhs)
{
  return ezStringUtils::Compare_NoCase(lhs.m_szTestName, rhs.m_szTestName) < 0;
}

/// Operator to sort sub-tests alphabetically
inline bool SortSubTest_Operator(const ezSubTestEntry& lhs, const ezSubTestEntry& rhs)
{
  return ezStringUtils::Compare_NoCase(lhs.m_szSubTestName, rhs.m_szSubTestName) < 0;
}

/// Sorts all tests and subtests alphabetically
void SortTestsAlphabetically(std::deque<ezTestEntry>& inout_tests)
{
  std::sort(inout_tests.begin(), inout_tests.end(), SortTest_Operator);

  for (ezUInt32 i = 0; i < inout_tests.size(); ++i)
    std::sort(inout_tests[i].m_SubTests.begin(), inout_tests[i].m_SubTests.end(), SortSubTest_Operator);
}

/// Writes the given test order to a simple config file
void SaveTestOrder(const char* szFile, const std::deque<ezTestEntry>& allTests)
{
  FILE* pFile = fopen(szFile, "wb");
  if (!pFile)
    return;

  char szTemp[256] = "";

  // Test order
  for (ezUInt32 t = 0; t < allTests.size(); ++t)
  {
    ezStringUtils::snprintf(szTemp, 256, "%s = %s\n", allTests[t].m_szTestName, allTests[t].m_bEnableTest ? "on" : "off");
    fwrite(szTemp, sizeof(char), strlen(szTemp), pFile);

    for (ezUInt32 st = 0; st < allTests[t].m_SubTests.size(); ++st)
    {
      ezStringUtils::snprintf(szTemp, 256, "  %s = %s\n", allTests[t].m_SubTests[st].m_szSubTestName, allTests[t].m_SubTests[st].m_bEnableTest ? "on" : "off");
      fwrite(szTemp, sizeof(char), strlen(szTemp), pFile);
    }
  }

  fclose(pFile);
}

/// Reads one line from a text file, strips away white-spaces. Returns true if the line originally started with a white-space (ie. was
/// indented).
inline bool ReadLine(FILE* pFile, char* pDest, ezUInt32 uiBufferSize)
{
  ezUInt32 iPos = 0;
  bool bIndented = false;

  while (iPos < (uiBufferSize - 1))
  {
    char c = '\0';
    if (fread(&c, sizeof(char), 1, pFile) == 0)
      break;

    if ((c == ' ') || (c == '\t'))
    {
      if (iPos == 0)
        bIndented = true;

      continue;
    }

    if (c == '\r')
      continue;

    if (c == '\n')
    {
      if (iPos > 0)
        break;

      continue;
    }

    pDest[iPos] = c;
    ++iPos;
  }

  pDest[iPos] = '\0';
  return bIndented;
}

/// Removes all spaces, tabs and \r characters from a string. Modifies it in place.
inline void StripWhitespaces(char* szString)
{
  ezUInt32 uiWritePos = 0;
  ezUInt32 uiReadPos = 0;

  while (szString[uiReadPos] != '\0')
  {
    if ((szString[uiReadPos] != ' ') && (szString[uiReadPos] != '\t') && (szString[uiReadPos] != '\r'))
    {
      szString[uiWritePos] = szString[uiReadPos];
      ++uiWritePos;
    }

    ++uiReadPos;
  }

  szString[uiWritePos] = '\0';
}

void LoadTestOrder(const char* szFile, std::deque<ezTestEntry>& ref_allTests)
{
  FILE* pFile = fopen(szFile, "rb");
  if (!pFile)
  {
    SortTestsAlphabetically(ref_allTests);
    return;
  }

  // If we do load a test order file only tests enabled in the file should be enabled.
  // Otherwise newly added tests would be enabled by default and carefully crafted
  // test order files would start running other tests they were not meant to run.
  for (ezTestEntry& test : ref_allTests)
  {
    test.m_bEnableTest = false;
    for (ezSubTestEntry& subTest : test.m_SubTests)
    {
      subTest.m_bEnableTest = false;
    }
  }

  ezInt32 iLastMainTest = 0;

  while (!feof(pFile))
  {
    char szTestName[256] = "";
    char szOtherName[256] = "";

    const bool bIndented = ReadLine(pFile, szTestName, 256);
    const bool bIsOff = strstr(szTestName, "=off") != nullptr;

    const char* pEnd = strstr(szTestName, "=");
    if (pEnd)
    {
      ezInt32 iPos = (ezInt32)(pEnd - szTestName);
      szTestName[iPos] = '\0';
    }

    if (szTestName[0] != '\0')
    {
      if (!bIndented)
      {
        iLastMainTest = -1;

        // Are we in the settings block?
        if (ezStringUtils::IsEqual_NoCase("Settings", szTestName))
        {
          iLastMainTest = -1;
        }
        // Are we in a test block?
        for (ezUInt32 t = 0; t < ref_allTests.size(); ++t)
        {
          strcpy(szOtherName, ref_allTests[t].m_szTestName);
          StripWhitespaces(szOtherName);

          if (ezStringUtils::IsEqual_NoCase(szOtherName, szTestName))
          {
            iLastMainTest = t;
            ref_allTests[t].m_bEnableTest = !bIsOff;
            break;
          }
        }
      }
      else
      {
        // We are in a test block
        if (iLastMainTest >= 0)
        {
          for (ezUInt32 t = 0; t < ref_allTests[iLastMainTest].m_SubTests.size(); ++t)
          {
            strcpy(szOtherName, ref_allTests[iLastMainTest].m_SubTests[t].m_szSubTestName);
            StripWhitespaces(szOtherName);

            if (ezStringUtils::IsEqual_NoCase(szOtherName, szTestName))
            {
              ref_allTests[iLastMainTest].m_SubTests[t].m_bEnableTest = !bIsOff;
              break;
            }
          }
        }
      }
    }
  }

  fclose(pFile);
  SortTestsAlphabetically(ref_allTests);
}

void SaveTestSettings(const char* szFile, TestSettings& ref_testSettings)
{
  FILE* pFile = fopen(szFile, "wb");
  if (!pFile)
    return;

  char szTemp[256] = "";

  // Settings
  ezStringUtils::snprintf(szTemp, 256, "Settings\n");
  fwrite(szTemp, sizeof(char), strlen(szTemp), pFile);
  {
    ezStringUtils::snprintf(szTemp, 256, "  AssertOnTestFail = %s\n", ref_testSettings.m_AssertOnTestFail != AssertOnTestFail::DoNotAssert ? "on" : "off");
    fwrite(szTemp, sizeof(char), strlen(szTemp), pFile);
    ezStringUtils::snprintf(szTemp, 256, "  OpenHtmlOutputOnError = %s\n", ref_testSettings.m_bOpenHtmlOutputOnError ? "on" : "off");
    fwrite(szTemp, sizeof(char), strlen(szTemp), pFile);
    ezStringUtils::snprintf(szTemp, 256, "  KeepConsoleOpen = %s\n", ref_testSettings.m_bKeepConsoleOpen ? "on" : "off");
    fwrite(szTemp, sizeof(char), strlen(szTemp), pFile);
    ezStringUtils::snprintf(szTemp, 256, "  ShowMessageBox = %s\n", ref_testSettings.m_bShowMessageBox ? "on" : "off");
    fwrite(szTemp, sizeof(char), strlen(szTemp), pFile);
    ezStringUtils::snprintf(szTemp, 256, "  DisableSuccessfulTests = %s\n", ref_testSettings.m_bAutoDisableSuccessfulTests ? "on" : "off");
    fwrite(szTemp, sizeof(char), strlen(szTemp), pFile);
  }

  fclose(pFile);
}

void LoadTestSettings(const char* szFile, TestSettings& ref_testSettings)
{
  FILE* pFile = fopen(szFile, "rb");
  if (!pFile)
  {
    return;
  }

  bool bInSettings = false;

  while (!feof(pFile))
  {
    char szTestName[256] = "";
    char szOtherName[256] = "";

    const bool bIndented = ReadLine(pFile, szTestName, 256);
    const bool bIsOff = strstr(szTestName, "=off") != nullptr;
    const char* pEnd = strstr(szTestName, "=");
    if (pEnd)
    {
      ezInt32 iPos = (ezInt32)(pEnd - szTestName);
      szTestName[iPos] = '\0';
    }

    if (szTestName[0] != '\0')
    {
      if (!bIndented)
      {
        // Are we in the settings block?
        bInSettings = ezStringUtils::IsEqual_NoCase("Settings", szTestName);
      }
      else
      {
        // We are in the settings block
        if (bInSettings)
        {
          if (ezStringUtils::IsEqual_NoCase("AssertOnTestFail", szTestName))
          {
            ref_testSettings.m_AssertOnTestFail = bIsOff ? AssertOnTestFail::DoNotAssert : AssertOnTestFail::AssertIfDebuggerAttached;
          }
          else if (ezStringUtils::IsEqual_NoCase("OpenHtmlOutputOnError", szTestName))
          {
            ref_testSettings.m_bOpenHtmlOutputOnError = !bIsOff;
          }
          else if (ezStringUtils::IsEqual_NoCase("KeepConsoleOpen", szTestName))
          {
            ref_testSettings.m_bKeepConsoleOpen = !bIsOff;
          }
          else if (ezStringUtils::IsEqual_NoCase("ShowMessageBox", szTestName))
          {
            ref_testSettings.m_bShowMessageBox = !bIsOff;
          }
          else if (ezStringUtils::IsEqual_NoCase("DisableSuccessfulTests", szTestName))
          {
            ref_testSettings.m_bAutoDisableSuccessfulTests = !bIsOff;
          }
        }
      }
    }
  }

  fclose(pFile);
}
