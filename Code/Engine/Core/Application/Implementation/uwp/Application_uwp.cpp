#include <PCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <Core/Application/Implementation/uwp/Application_uwp.h>
#include <Core/Application/Application.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/IO/OSFile.h>

#include <windows.ui.core.h>
#include <wrl/event.h>

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

HRESULT ezUwpApplication::Initialize(ICoreApplicationView* applicationView)
{
  using OnActivatedHandler = __FITypedEventHandler_2_Windows__CApplicationModel__CCore__CCoreApplicationView_Windows__CApplicationModel__CActivation__CIActivatedEventArgs;
  EZ_SUCCEED_OR_RETURN(applicationView->add_Activated(Microsoft::WRL::Callback<OnActivatedHandler>(this, &ezUwpApplication::OnActivated).Get(), &m_activateRegistrationToken));

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

HRESULT ezUwpApplication::OnActivated(ICoreApplicationView* view, IActivatedEventArgs* args)
{
  view->remove_Activated(m_activateRegistrationToken);

  ActivationKind activationKind;
  EZ_SUCCEED_OR_RETURN(args->get_Kind(&activationKind));

  if (activationKind == ActivationKind_Launch)
  {
    ComPtr<ILaunchActivatedEventArgs> launchArgs;
    EZ_SUCCEED_OR_RETURN(args->QueryInterface(launchArgs.GetAddressOf()));

    HString argHString;
    EZ_SUCCEED_OR_RETURN(launchArgs->get_Arguments(argHString.GetAddressOf()));

    ezDynamicArray<const char*> argv;
    ezCommandLineUtils::SplitCommandLineString(ezStringUtf8(argHString).GetData(), true, m_commandLineArgs, argv);

    m_application->SetCommandLineArguments(argv.GetCount(), argv.GetData());
  }

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

