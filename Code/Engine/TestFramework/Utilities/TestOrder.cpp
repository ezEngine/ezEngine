#include <TestFramework/PCH.h>
#include <TestFramework/Utilities/TestOrder.h>
#include <Foundation/Math/Math.h>
#include <algorithm>

/// Operator to sort tests alphabetically
inline bool SortTest_Operator(const ezTestEntry& lhs, const ezTestEntry& rhs)
{
  return strcmp (lhs.m_szTestName, rhs.m_szTestName) < 0;
}

/// Operator to sort sub-tests alphabetically
inline bool SortSubTest_Operator(const ezSubTestEntry& lhs, const ezSubTestEntry& rhs)
{
  return strcmp (lhs.m_szSubTestName, rhs.m_szSubTestName) < 0;
}

/// Sorts all tests and subtests alphabetically
void SortTestsAlphabetically(std::deque<ezTestEntry>& inout_Tests)
{
  std::sort(inout_Tests.begin(), inout_Tests.end(), SortTest_Operator);

  for (ezUInt32 i = 0; i < inout_Tests.size(); ++i)
    std::sort(inout_Tests[i].m_SubTests.begin(), inout_Tests[i].m_SubTests.end(), SortSubTest_Operator);
}

/// Writes the given test order to a simple config file
void SaveTestOrder(const char* szFile, const std::deque<ezTestEntry>& AllTests, TestSettings& testSettings)
{
  FILE* pFile = fopen(szFile, "wb");
  if (!pFile)
    return;

  char szTemp[256] = "";

  // Settings
  sprintf(szTemp, "Settings\n");
  fwrite(szTemp, sizeof(char), strlen(szTemp), pFile);
  {
    sprintf(szTemp, "  AssertOnTestFail = %s\n", testSettings.m_bAssertOnTestFail ? "on" : "off");
    fwrite(szTemp, sizeof(char), strlen(szTemp), pFile);
    sprintf(szTemp, "  OpenHtmlOutput = %s\n", testSettings.m_bOpenHtmlOutput ? "on" : "off");
    fwrite(szTemp, sizeof(char), strlen(szTemp), pFile);
    sprintf(szTemp, "  KeepConsoleOpen = %s\n", testSettings.m_bKeepConsoleOpen ? "on" : "off");
    fwrite(szTemp, sizeof(char), strlen(szTemp), pFile);
    sprintf(szTemp, "  ShowMessageBox = %s\n", testSettings.m_bShowMessageBox ? "on" : "off");
    fwrite(szTemp, sizeof(char), strlen(szTemp), pFile);
  }

  // Test order
  for (ezUInt32 t = 0; t < AllTests.size(); ++t)
  {
    sprintf(szTemp, "%s = %s\n", AllTests[t].m_szTestName, AllTests[t].m_bEnableTest ? "on" : "off");
    fwrite(szTemp, sizeof(char), strlen(szTemp), pFile);

    for (ezUInt32 st = 0; st < AllTests[t].m_SubTests.size(); ++st)
    {
      sprintf(szTemp, "  %s = %s\n", AllTests[t].m_SubTests[st].m_szSubTestName, AllTests[t].m_SubTests[st].m_bEnableTest ? "on" : "off");
      fwrite(szTemp, sizeof(char), strlen(szTemp), pFile);
    }
  }

  fclose(pFile);
}

/// Reads one line from a text file, strips away white-spaces. Returns true if the line originally started with a white-space (ie. was indented).
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

void LoadTestOrder(const char* szFile, std::deque<ezTestEntry>& AllTests, TestSettings& testSettings)
{
  FILE* pFile = fopen(szFile, "rb");
  if (!pFile)
  {
    SortTestsAlphabetically(AllTests);
    return;
  }

  ezInt32 iLastMainTest =  0;

  while (!feof(pFile))
  {
    char szTestName[256] = "";
    char szOtherName[256] = "";

    const bool bIndented = ReadLine(pFile, szTestName, 256);
    const bool bIsOff = strstr(szTestName, "=off") != nullptr;

    const char* pEnd = strstr(szTestName, "=");
    if (pEnd)
    {
      ezInt32 iPos = (ezInt32) (pEnd - szTestName);
      szTestName[iPos] = '\0';
    }

    if (szTestName[0] != '\0')
    {
      if (!bIndented)
      {
        iLastMainTest = -1;

        // Are we in the settings block?
        if (strcmp("Settings", szTestName) == 0)
        {
          iLastMainTest = -1;
        }
        // Are we in a test block?
        for (ezUInt32 t = 0; t < AllTests.size(); ++t)
        {
          strcpy(szOtherName, AllTests[t].m_szTestName);
          StripWhitespaces(szOtherName);

          if (strcmp(szOtherName, szTestName) == 0)
          {
            iLastMainTest = t;
            AllTests[t].m_bEnableTest = !bIsOff;
            break;
          }
        }
      }
      else
      {
        // We are in the settings block
        if (iLastMainTest == -1)
        {
          if (strcmp("AssertOnTestFail", szTestName) == 0)
          {
            testSettings.m_bAssertOnTestFail = !bIsOff;
          }
          else if (strcmp("OpenHtmlOutput", szTestName) == 0)
          {
            testSettings.m_bOpenHtmlOutput = !bIsOff;
          }
          else if (strcmp("KeepConsoleOpen", szTestName) == 0)
          {
            testSettings.m_bKeepConsoleOpen = !bIsOff;
          }
          else if (strcmp("ShowMessageBox", szTestName) == 0)
          {
            testSettings.m_bShowMessageBox = !bIsOff;
          }
        }
        // We are in a test block
        else if (iLastMainTest >= 0)
        {
          for (ezUInt32 t = 0; t < AllTests[iLastMainTest].m_SubTests.size(); ++t)
          {
            strcpy(szOtherName, AllTests[iLastMainTest].m_SubTests[t].m_szSubTestName);
            StripWhitespaces(szOtherName);

            if (strcmp(szOtherName, szTestName) == 0)
            {
              AllTests[iLastMainTest].m_SubTests[t].m_bEnableTest = !bIsOff;
              break;
            }
          }
        }
      }
    }
  }

  fclose(pFile);
  SortTestsAlphabetically(AllTests);
}



EZ_STATICLINK_FILE(TestFramework, TestFramework_Utilities_TestOrder);

