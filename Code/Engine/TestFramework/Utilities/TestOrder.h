#pragma once

#include <TestFramework/Framework/TestFramework.h>
#include <Foundation/Math/Math.h>
#include <algorithm>

struct TestSettings
{
  TestSettings()
  {
    m_bAssertOnTestFail = false;
    m_bOpenHtmlOutput = false;
    m_bKeepConsoleOpen = false;
    m_bShowMessageBox = true;
    m_bRunTests = false;
    m_bNoSaving = false;
    m_bCloseOnSuccess = false;
    m_bNoGUI = false;
    m_iRevision = -1;
    m_bEnableAllTests = false;
  }

  // The following settings are stored in the settings file.
  bool m_bAssertOnTestFail;
  bool m_bOpenHtmlOutput;
  bool m_bKeepConsoleOpen;
  bool m_bShowMessageBox;
  
  // The following settings are only set via command-line.
  bool m_bRunTests;         /// Only needed for GUI applications, in console mode tests are always run automatically.
  bool m_bNoSaving;         /// Allows to run the test with settings through the command line without saving those settings for later.
  bool m_bCloseOnSuccess;   /// Closes the application upon success immediately.
  bool m_bNoGUI;            /// Starts the tests in console mode, test are started automatically.
  int m_iRevision;          /// Revision in the RCS of this test run. Will be written into the test results json file for later reference.
  std::string m_sJsonOutput;/// Absolute path to the json file the results should be written to.
  bool m_bEnableAllTests;   /// Enables all test.
};

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
inline void SortTestsAlphabetically(std::deque<ezTestEntry>& inout_Tests)
{
  std::sort(inout_Tests.begin(), inout_Tests.end(), SortTest_Operator);

  for (ezUInt32 i = 0; i < inout_Tests.size(); ++i)
    std::sort(inout_Tests[i].m_SubTests.begin(), inout_Tests[i].m_SubTests.end(), SortSubTest_Operator);
}

/// Writes the given test order to a simple config file
inline void SaveTestOrder(const char* szFile, const std::deque<ezTestEntry>& AllTests, TestSettings& testSettings)
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

/// Loads the order of tests from a file and sorts the existing tests in the array accordingly. The file and the array do not need to contain the same set of tests, the array is only re-arranged, nothing is added or removed from it.
inline void LoadTestOrder(const char* szFile, std::deque<ezTestEntry>& AllTests, TestSettings& testSettings)
{
  FILE* pFile = fopen(szFile, "rb");
  if (!pFile)
  {
    SortTestsAlphabetically(AllTests);
    return;
  }

  ezInt32 iMainTestPos = 0;
  ezInt32 iSubTestPos = 0;
  ezInt32 iLastMainTest =  0;

  while (!feof(pFile))
  {
    char szTestName[256] = "";
    char szOtherName[256] = "";

    const bool bIndented = ReadLine(pFile, szTestName, 256);
    const bool bIsOff = strstr(szTestName, "=off") != NULL;

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
        iSubTestPos = 0;
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
            // swap the two tests, to move the currently found test to the front
            ezMath::Swap(AllTests[iMainTestPos], AllTests[t]);

            AllTests[iMainTestPos].m_bEnableTest = !bIsOff;

            iLastMainTest = iMainTestPos;
            ++iMainTestPos;
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
              // swap the two tests, to move the currently found test to the front
              ezSubTestEntry entry = AllTests[iLastMainTest].m_SubTests[iSubTestPos];
              AllTests[iLastMainTest].m_SubTests[iSubTestPos] = AllTests[iLastMainTest].m_SubTests[t];
              AllTests[iLastMainTest].m_SubTests[t] = entry;

              AllTests[iLastMainTest].m_SubTests[iSubTestPos].m_bEnableTest = !bIsOff;
              break;
            }
          }
        }
        ++iSubTestPos;
      }
    }
  }

  fclose(pFile);
  SortTestsAlphabetically(AllTests);
}