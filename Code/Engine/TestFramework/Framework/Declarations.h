#pragma once

#include <TestFramework/Basics.h>
#include <deque>
#include <string>

class ezTestFramework;
class ezTestBaseClass;

/// \brief Stores the identification of a sub-test.
struct ezSubTestEntry
{
  ezSubTestEntry() : m_iSubTestIdentifier(-1), m_szSubTestName(""), m_bEnableTest(true) { }

  ezInt32 m_iSubTestIdentifier;
  const char* m_szSubTestName;
  bool m_bEnableTest;
};

/// \brief Stores the identification of a test.
struct ezTestEntry
{
  ezTestEntry() : m_pTest(nullptr), m_szTestName(""), m_bEnableTest(true) { }

  ezTestBaseClass* m_pTest;
  const char* m_szTestName;
  std::deque<ezSubTestEntry> m_SubTests;
  bool m_bEnableTest;
};

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
    m_uiFullPasses = 1;
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
  ezUInt8 m_uiFullPasses;   /// All tests are done this often, to check whether some tests fail only when executed multiple times.
};
