#include <TestFramework/PCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <TestFramework/Framework/uwp/uwpTestFramework.h>
#include <TestFramework/Framework/uwp/uwpTestApplication.h>

#include <Foundation/Logging/Log.h>

#include <wrl/wrappers/corewrappers.h>
#include <windows.foundation.h>

using namespace Microsoft::WRL::Wrappers;

ezUwpTestFramework::ezUwpTestFramework(const char* szTestName, const char* szAbsTestDir, int argc, const char** argv)
  : ezTestFramework(szTestName, szAbsTestDir, argc, argv)
{
  // Todo: Should RoInitialize/RoUninitialize be moved to foundation?
  if (FAILED(RoInitialize(RO_INIT_MULTITHREADED)))
  {
    ezLog::Error("Failed to init WinRT.");
  }

  m_application = Make<ezUwpTestApplication>(*this);
}

ezUwpTestFramework::~ezUwpTestFramework()
{
  RoUninitialize();
}

void ezUwpTestFramework::Run()
{
  ComPtr<ABI::Windows::ApplicationModel::Core::ICoreApplication> coreApplication;
  HRESULT result = ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(), &coreApplication);
  if (FAILED(result))
  {
    ezLog::Error("Failed to create core application.");
    return;
  }
  else
  {
    coreApplication->Run(m_application.Get());
  }
}

#endif

EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_uwp_uwpTestFramework);

