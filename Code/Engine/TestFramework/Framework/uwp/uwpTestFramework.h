#pragma once

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

#include <TestFramework/Basics.h>
#include <TestFramework/Framework/TestFramework.h>

#include <Foundation/Basics/Platform/uwp/UWPUtils.h>

/// \brief Derived ezTestFramework which signals the GUI to update whenever a new tests result comes in.
class EZ_TEST_DLL ezUwpTestFramework : public ezTestFramework
{
public:
  ezUwpTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int argc, const char** argv);
  virtual ~ezUwpTestFramework();

  ezUwpTestFramework(ezUwpTestFramework&) = delete;
  void operator=(ezUwpTestFramework&) = delete;

  void Run();
};

#endif
