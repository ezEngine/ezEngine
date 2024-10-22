#pragma once

#include <Foundation/FoundationInternal.h>
EZ_FOUNDATION_INTERNAL_HEADER

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

#  include <Foundation/Basics.h>
#  include <Foundation/Strings/String.h>

#  include <Foundation/Platform/UWP/Utils/UWPUtils.h>
#  include <winrt/base.h>

#  include <winrt/Windows.ApplicationModel.Activation.h>
#  include <winrt/Windows.ApplicationModel.Core.h>

class ezApplication;

/// Minimal implementation of a uwp application.
class ezUwpApplication : public winrt::implements<ezUwpApplication, winrt::Windows::ApplicationModel::Core::IFrameworkView, winrt::Windows::ApplicationModel::Core::IFrameworkViewSource>
{
public:
  ezUwpApplication(ezApplication* application);
  virtual ~ezUwpApplication();

  // Inherited via IFrameworkViewSource
  winrt::Windows::ApplicationModel::Core::IFrameworkView CreateView();

  // Inherited via IFrameworkView
  void Initialize(winrt::Windows::ApplicationModel::Core::CoreApplicationView const& applicationView);
  void SetWindow(winrt::Windows::UI::Core::CoreWindow const& window);
  void Load(winrt::hstring const& entryPoint);
  void Run();
  void Uninitialize();

private:
  // Application lifecycle event handlers.
  void OnViewActivated(winrt::Windows::ApplicationModel::Core::CoreApplicationView const& sender, winrt::Windows::ApplicationModel::Activation::IActivatedEventArgs const& args);

  winrt::event_token m_activateRegistrationToken;

  ezApplication* m_application;
  ezDynamicArray<ezString> m_commandLineArgs;
};

#endif
