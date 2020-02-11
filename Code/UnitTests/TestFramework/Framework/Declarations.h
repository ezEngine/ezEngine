#pragma once

#include <Foundation/Types/Status.h>
#include <TestFramework/TestFrameworkDLL.h>
#include <deque>
#include <string>

class ezTestFramework;
class ezTestBaseClass;

/// \brief Stores the identification of a sub-test.
struct ezSubTestEntry
{
  ezSubTestEntry()
    : m_iSubTestIdentifier(-1)
    , m_szSubTestName("")
    , m_bEnableTest(true)
  {
  }

  ezInt32 m_iSubTestIdentifier;
  const char* m_szSubTestName;
  bool m_bEnableTest;
};

/// \brief Stores the identification of a test.
struct ezTestEntry
{
  ezTestEntry()
    : m_pTest(nullptr)
    , m_szTestName("")
    , m_bEnableTest(true)
  {
  }

  ezTestBaseClass* m_pTest;
  const char* m_szTestName;
  std::deque<ezSubTestEntry> m_SubTests;
  std::string m_sNotAvailableReason;
  bool m_bEnableTest;
};

struct TestSettings
{
  // The following settings are stored in the settings file.
  bool m_bAssertOnTestFail = false;
  bool m_bOpenHtmlOutputOnError = false;
  bool m_bKeepConsoleOpen = false;
  bool m_bShowTimestampsInLog = false;
  bool m_bShowMessageBox = false;
  bool m_bAutoDisableSuccessfulTests = false;

  // The following settings are only set via command-line.
  bool m_bRunTests = false; /// Only needed for GUI applications, in console mode tests are always run automatically.
  bool m_bNoAutomaticSaving =
    false;                        /// Allows to run the test with settings through the command line without saving those settings for later.
  bool m_bCloseOnSuccess = false; /// Closes the application upon success immediately.
  bool m_bNoGUI = false;          /// Starts the tests in console mode, test are started automatically.
  int m_iRevision = -1;      /// Revision in the RCS of this test run. Will be written into the test results json file for later reference.
  std::string m_sJsonOutput; /// Absolute path to the json file the results should be written to.
  bool m_bEnableAllTests = false; /// Enables all test.
  ezUInt8 m_uiFullPasses = 1;     /// All tests are done this often, to check whether some tests fail only when executed multiple times.
};
