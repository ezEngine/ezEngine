#include <PCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#include <Core/Application/Implementation/uwp/Application_uwp.h>
#include <Core/Application/Application.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/IO/OSFile.h>

#include <wrl/event.h>
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

HRESULT ezUwpApplication::Initialize(ICoreApplicationView* applicationView)
{
  typedef __FITypedEventHandler_2_Windows__CApplicationModel__CCore__CCoreApplicationView_Windows__CApplicationModel__CActivation__CIActivatedEventArgs ActivatedHandler;
  EZ_SUCCEED_OR_RETURN_HRESULT(applicationView->add_Activated(Callback<ActivatedHandler>(this, &ezUwpApplication::OnActivated).Get(), &m_activateRegistrationToken));

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

HRESULT ezUwpApplication::OnActivated(ABI::Windows::ApplicationModel::Core::ICoreApplicationView* view, ABI::Windows::ApplicationModel::Activation::IActivatedEventArgs* args)
{
  ABI::Windows::ApplicationModel::Activation::ActivationKind activationKind;
  EZ_SUCCEED_OR_RETURN_HRESULT(args->get_Kind(&activationKind));

  if (activationKind == ABI::Windows::ApplicationModel::Activation::ActivationKind_Launch)
  {
    ComPtr<ABI::Windows::ApplicationModel::Activation::ILaunchActivatedEventArgs> launchArgs;
    EZ_SUCCEED_OR_RETURN_HRESULT(args->QueryInterface(launchArgs.GetAddressOf()));
    HString argHString;
    EZ_SUCCEED_OR_RETURN_HRESULT(launchArgs->get_Arguments(argHString.GetAddressOf()));

    // Add application dir as first argument as customary on other platforms.
    m_commandLineArgs.Clear();
    wchar_t moduleFilename[256];
    GetModuleFileNameW(nullptr, moduleFilename, 256);
    m_commandLineArgs.PushBack(ezStringUtf8(moduleFilename).GetData());
    // Simple args splitting. Not as powerful as Win32's CommandLineToArgvW.
    ezStringUtf8 argUtf8String(argHString);
    const char* currentChar = argUtf8String.GetData();
    const char* lastEnd = currentChar;
    bool inQuotes = false;
    while (*currentChar != '\0')
    {
      if (*currentChar == '\"')
        inQuotes = !inQuotes;
      else if (*currentChar == ' ' && !inQuotes)
      {
        m_commandLineArgs.PushBack(ezStringView(lastEnd, currentChar));
        lastEnd = currentChar;
      }
      ezUnicodeUtils::MoveToNextUtf8(currentChar);
    }

    ezDynamicArray<const char*> argv;
    argv.Reserve(m_commandLineArgs.GetCount());
    for (ezString& str : m_commandLineArgs)
      argv.PushBack(str.GetData());

    m_application->SetCommandLineArguments(argv.GetCount(), argv.GetData());

    view->remove_Activated(m_activateRegistrationToken);
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

