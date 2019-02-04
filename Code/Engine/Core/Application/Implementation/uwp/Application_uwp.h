#pragma once

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

#include <Core/Basics.h>
#include <Foundation/Strings/String.h>

#include <Windows.ApplicationModel.core.h>
#include <Windows.Applicationmodel.h>

#include <Foundation/Basics/Platform/uwp/UWPUtils.h>

using namespace ABI::Windows::ApplicationModel::Core;
using namespace ABI::Windows::ApplicationModel::Activation;

class ezApplication;

/// Minimal implementation of a uwp application.
class ezUwpApplication : public RuntimeClass<IFrameworkViewSource, IFrameworkView>
{
public:
  ezUwpApplication(ezApplication* application);
  virtual ~ezUwpApplication();

  // Inherited via IFrameworkViewSource
  virtual HRESULT __stdcall CreateView(IFrameworkView** viewProvider) override;

  // Inherited via IFrameworkView
  virtual HRESULT __stdcall Initialize(ICoreApplicationView* applicationView) override;
  virtual HRESULT __stdcall SetWindow(ABI::Windows::UI::Core::ICoreWindow* window) override;
  virtual HRESULT __stdcall Load(HSTRING entryPoint) override;
  virtual HRESULT __stdcall Run() override;
  virtual HRESULT __stdcall Uninitialize() override;

private:
  HRESULT OnActivated(ICoreApplicationView*, IActivatedEventArgs* args);

  ezApplication* m_application;
  EventRegistrationToken m_activateRegistrationToken;
  ezDynamicArray<ezString> m_commandLineArgs;
};

#endif

