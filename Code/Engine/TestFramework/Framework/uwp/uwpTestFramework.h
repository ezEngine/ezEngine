#pragma once
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

#include <TestFramework/Basics.h>
#include <TestFramework/Framework/TestFramework.h>

// For ComPtr
#include <wrl/client.h>

class ezUwpTestApplication;

/// \brief Derived ezTestFramework which signals the GUI to update whenever a new tests result comes in.
class EZ_TEST_DLL ezUwpTestFramework : public ezTestFramework
{
public:
  ezUwpTestFramework(const char* szTestName, const char* szAbsTestDir, int argc, const char** argv);
  virtual ~ezUwpTestFramework();

  void Run();

private:
  ezUwpTestFramework(ezUwpTestFramework&) = delete;
  void operator=(ezUwpTestFramework&) = delete;

  Microsoft::WRL::ComPtr<ezUwpTestApplication> m_application;
};

#endif

