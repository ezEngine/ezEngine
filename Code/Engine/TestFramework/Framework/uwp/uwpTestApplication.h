#pragma once

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

#include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#include <Windows.ApplicationModel.ExtendedExecution.h>
#include <Windows.ApplicationModel.core.h>
#include <Windows.Applicationmodel.h>

using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Windows::ApplicationModel::Activation;
using namespace ABI::Windows::ApplicationModel::ExtendedExecution;

class ezTestFramework;

class ezUwpTestApplication : public RuntimeClass<IFrameworkViewSource, IFrameworkView>
{
public:
  ezUwpTestApplication(ezTestFramework& testFramework);
  virtual ~ezUwpTestApplication();

  // Inherited via IFrameworkViewSource
  virtual HRESULT __stdcall CreateView(IFrameworkView** viewProvider) override;

  // Inherited via IFrameworkView
  virtual HRESULT __stdcall Initialize(ICoreApplicationView* applicationView) override;
  virtual HRESULT __stdcall SetWindow(ABI::Windows::UI::Core::ICoreWindow* window) override;
  virtual HRESULT __stdcall Load(HSTRING entryPoint) override;
  virtual HRESULT __stdcall Run() override;
  virtual HRESULT __stdcall Uninitialize() override;

private:
  // Events
  HRESULT OnActivated(ICoreApplicationView* applicationView, IActivatedEventArgs* args);
  HRESULT OnSessionRevoked(IInspectable* sender, IExtendedExecutionRevokedEventArgs* args);

  ezTestFramework& m_testFramework;
  EventRegistrationToken m_eventRegistrationOnActivate;
  EventRegistrationToken m_eventRegistrationOnRevokedSession;
  ComPtr<IExtendedExecutionSession> m_extendedExecutionSession;
};

#endif
