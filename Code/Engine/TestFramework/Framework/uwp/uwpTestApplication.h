#pragma once
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)

#include <Windows.Applicationmodel.h>
#include <Windows.ApplicationModel.core.h>
#include <wrl/implements.h>

using namespace Microsoft::WRL;
using namespace ABI::Windows::ApplicationModel::Core;

class ezTestFramework;

class ezUwpTestApplication : public RuntimeClass<IFrameworkViewSource, IFrameworkView>
{
public:
  ezUwpTestApplication(ezTestFramework& testFramework);
  virtual ~ezUwpTestApplication();

  // Inherited via IFrameworkViewSource
  virtual HRESULT CreateView(IFrameworkView** viewProvider) override;

  // Inherited via IFrameworkView
  virtual HRESULT Initialize(ICoreApplicationView * applicationView) override;
  virtual HRESULT SetWindow(ABI::Windows::UI::Core::ICoreWindow* window) override;
  virtual HRESULT Load(HSTRING entryPoint) override;
  virtual HRESULT Run() override;
  virtual HRESULT Uninitialize() override;

private:
  ezTestFramework& m_testFramework;
};

#endif

