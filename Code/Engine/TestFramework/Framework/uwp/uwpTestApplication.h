#pragma once
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

#include <Windows.Applicationmodel.h>
#include <Windows.ApplicationModel.core.h>

#include <Foundation/Basics/Platform/uwp/UWPUtils.h>

using namespace ABI::Windows::ApplicationModel::Core;

class ezTestFramework;

class ezUwpTestApplication : public RuntimeClass<IFrameworkViewSource, IFrameworkView>
{
public:
  ezUwpTestApplication(ezTestFramework& testFramework);
  virtual ~ezUwpTestApplication();

  // Inherited via IFrameworkViewSource
  virtual HRESULT __stdcall CreateView(IFrameworkView** viewProvider) override;

  // Inherited via IFrameworkView
  virtual HRESULT __stdcall Initialize(ICoreApplicationView * applicationView) override;
  virtual HRESULT __stdcall SetWindow(ABI::Windows::UI::Core::ICoreWindow* window) override;
  virtual HRESULT __stdcall Load(HSTRING entryPoint) override;
  virtual HRESULT __stdcall Run() override;
  virtual HRESULT __stdcall Uninitialize() override;

private:
  ezTestFramework& m_testFramework;
};

#endif

