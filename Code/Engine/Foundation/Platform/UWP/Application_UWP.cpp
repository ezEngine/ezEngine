#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#  include <Foundation/Application/Application.h>
#  include <Foundation/Application/Implementation/uwp/Application_uwp.h>
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Strings/StringConversion.h>

// Disable warning produced by CppWinRT
#  pragma warning(disable : 5205)
#  include <winrt/Windows.ApplicationModel.Activation.h>
#  include <winrt/Windows.ApplicationModel.Core.h>
#  include <winrt/Windows.Foundation.Collections.h>
#  include <winrt/Windows.Foundation.h>
#  include <winrt/Windows.UI.Core.h>

using namespace winrt::Windows::ApplicationModel::Core;

ezUwpApplication::ezUwpApplication(ezApplication* application)
  : m_application(application)
{
}

ezUwpApplication::~ezUwpApplication() {}

winrt::Windows::ApplicationModel::Core::IFrameworkView ezUwpApplication::CreateView()
{
  return this->get_strong().try_as<winrt::Windows::ApplicationModel::Core::IFrameworkView>();
}

void ezUwpApplication::Initialize(winrt::Windows::ApplicationModel::Core::CoreApplicationView const& applicationView)
{
  applicationView.Activated({this, &ezUwpApplication::OnViewActivated});
}

void ezUwpApplication::SetWindow(winrt::Windows::UI::Core::CoreWindow const& window)
{
  EZ_IGNORE_UNUSED(window);
}

void ezUwpApplication::Load(winrt::hstring const& entryPoint)
{
  EZ_IGNORE_UNUSED(entryPoint);
}

void ezUwpApplication::Run()
{
  if (ezRun_Startup(m_application).Succeeded())
  {
    auto window = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
    window.Activate();

    ezRun_MainLoop(m_application);
  }
  ezRun_Shutdown(m_application);
}

void ezUwpApplication::Uninitialize()
{
}

void ezUwpApplication::OnViewActivated(winrt::Windows::ApplicationModel::Core::CoreApplicationView const& sender, winrt::Windows::ApplicationModel::Activation::IActivatedEventArgs const& args)
{
  sender.Activated(m_activateRegistrationToken);

  if (args.Kind() == winrt::Windows::ApplicationModel::Activation::ActivationKind::Launch)
  {
    auto launchArgs = args.as<winrt::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs>();
    winrt::hstring argHString = launchArgs.Arguments();

    ezDynamicArray<const char*> argv;
    ezCommandLineUtils::SplitCommandLineString(ezStringUtf8(argHString.c_str()).GetData(), true, m_commandLineArgs, argv);

    m_application->SetCommandLineArguments(argv.GetCount(), argv.GetData());
  }
}

EZ_FOUNDATION_DLL ezResult ezUWPRun(ezApplication* pApp)
{
  {
    auto application = winrt::make<ezUwpApplication>(pApp);
    winrt::Windows::ApplicationModel::Core::CoreApplication::Run(application.as<winrt::Windows::ApplicationModel::Core::IFrameworkViewSource>());
  }

  return EZ_SUCCESS;
}

#endif


