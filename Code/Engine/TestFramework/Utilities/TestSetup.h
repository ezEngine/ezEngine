#pragma once

#include <TestFramework/Basics.h>

class ezTestFramework;

/// \brief A collection of static helper functions to setup the test framework.
class EZ_TEST_DLL ezTestSetup
{
public:
  /// \brief Creates and returns a test framework with the given name.
  static ezTestFramework* InitTestFramework(const char* szTestName, const char* szNiceTestName);

  /// \brief Runs tests and returns number of errors.
  static ezInt32 RunTests(int argc, char **argv);

  /// \brief Deletes the test framework and outputs final test output.
  static void DeInitTestFramework();
};
