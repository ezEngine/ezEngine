#pragma once

#include <Foundation/Types/Status.h>
#include <TestFramework/TestFrameworkDLL.h>
#include <deque>
#include <string>

class ezTestFramework;
class ezTestBaseClass;

/// Stores the identification of a sub-test.
struct ezSubTestEntry
{
  ezSubTestEntry() = default;

  ezInt32 m_iSubTestIdentifier = -1;
  const char* m_szSubTestName = "";
  bool m_bEnableTest = true;
};

/// Stores the identification of a test.
struct ezTestEntry
{
  ezTestEntry() = default;

  ezTestBaseClass* m_pTest = nullptr;
  const char* m_szTestName = "";
  std::deque<ezSubTestEntry> m_SubTests;
  std::string m_sNotAvailableReason;
  bool m_bEnableTest = true;
};

enum class AssertOnTestFail
{
  DoNotAssert,
  AssertIfDebuggerAttached,
  AlwaysAssert,
};

struct TestSettings
{
  // The following settings are stored in the settings file.
  AssertOnTestFail m_AssertOnTestFail = AssertOnTestFail::DoNotAssert;
  bool m_bShowTimestampsInLog = false;
  bool m_bAutoDisableSuccessfulTests = false;

  // The following settings are only set via command-line.
  bool m_bSaveState = true;  ///< Whether the test order and settings are written back to disk. Off in console mode and whenever a custom order or settings file was given.
  bool m_bNoGUI = false;     ///< Starts the tests in console mode, tests are started automatically and the process exits when they are done.
  bool m_bListTests = false; ///< List all test names and exit.
  bool m_bShowHelp = false;  ///< Show command line help.
  int m_iRevision = -1;      ///< Revision in the RCS of this test run. Will be written into the test results json file for later reference.
  std::string m_sJsonOutput; ///< Absolute path to the json file the results should be written to.
  std::string m_sTestFilter; ///< Filter for which tests to run. A 'contains' check, or a full wildcard match if it contains '*' or '?'. Case insensitive.
};
