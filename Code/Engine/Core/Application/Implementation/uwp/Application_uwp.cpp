#include <Core/PCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <Core/Application/Implementation/uwp/Application_uwp.h>
#include <Core/Application/Application.h>

#include <windows.ui.core.h>

ezUwpApplication::ezUwpApplication(ezApplication* application)
  : m_application(application)
{
}

ezUwpApplication::~ezUwpApplication()
{
}

HRESULT ezUwpApplication::CreateView(IFrameworkView** viewProvider)
{
  *viewProvider = this;
  return S_OK;
}

HRESULT ezUwpApplication::Initialize(ICoreApplicationView * applicationView)
{
  return S_OK;
}

HRESULT ezUwpApplication::SetWindow(ABI::Windows::UI::Core::ICoreWindow * window)
{
  return S_OK;
}

HRESULT ezUwpApplication::Load(HSTRING entryPoint)
{
  return S_OK;
}

HRESULT ezUwpApplication::Run()
{
  ezRun(m_application);
  return S_OK;
}

HRESULT ezUwpApplication::Uninitialize()
{
  return S_OK;
}

EZ_CORE_DLL ezResult ezUWPRun(ezApplication* pApp)
{
  ComPtr<ABI::Windows::ApplicationModel::Core::ICoreApplication> coreApplication;
  HRESULT result = ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(), &coreApplication);
  if (FAILED(result))
  {
    printf("Failed to create core application: %i\n", result);
    return EZ_FAILURE;
  }

  {
    ComPtr<ezUwpApplication> application = Make<ezUwpApplication>(pApp);
    coreApplication->Run(application.Get());
    application.Detach();      // Was already deleted by uwp.
  }

  return EZ_SUCCESS;
}

#endif

EZ_STATICLINK_FILE(TestFramework, TestFramework_Framework_uwp_uwpTestFramework);

