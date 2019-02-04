#pragma once

#include <TestFramework/Basics.h>

class ezTestFramework;

/// \brief A collection of static helper functions to setup the test framework.
class EZ_TEST_DLL ezTestSetup
{
public:
  /// \brief Creates and returns a test framework with the given name.
  static ezTestFramework* InitTestFramework(const char* szTestName, const char* szNiceTestName, int argc, const char** argv);

  /// \brief Runs tests and returns number of errors.
  static ezTestAppRun RunTests();

  static ezInt32 GetFailedTestCount();

  /// \brief Deletes the test framework and outputs final test output.
  static void DeInitTestFramework();

private:
  static int s_argc;
  static const char** s_argv;
};

