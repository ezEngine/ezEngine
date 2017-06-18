#include <PCH.h>
#include <WindowsMixedReality/HolographicSpace.h>
#include <System/Window/Window.h>

#include <windows.graphics.holographic.h>
#include <windows.system.profile.h>

EZ_IMPLEMENT_SINGLETON(ezWindowsHolographicSpace);

EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, WindowsHolographicSpace)

ON_CORE_STARTUP
{
  EZ_DEFAULT_NEW(ezWindowsHolographicSpace);
}

ON_CORE_SHUTDOWN
{
  ezWindowsHolographicSpace* pDummy = ezWindowsHolographicSpace::GetSingleton();
  EZ_DEFAULT_DELETE(pDummy);
}

ON_ENGINE_STARTUP
{
}

ON_ENGINE_SHUTDOWN
{
}

EZ_END_SUBSYSTEM_DECLARATION


ezWindowsHolographicSpace::ezWindowsHolographicSpace()
  : m_SingletonRegistrar(this)
{
  if (FAILED(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_Graphics_Holographic_HolographicSpace).Get(), &m_pHolographicSpaceStatics)))
  {
    ezLog::Error("Failed to query HolographicSpace activation factory. Windows holographic won't be supported!");
  }
}

ezWindowsHolographicSpace::~ezWindowsHolographicSpace()
{
}

ezResult ezWindowsHolographicSpace::InitForWindow(const ezWindow& window)
{
  static_assert(std::is_same<ezWindowHandle, IUnknown*>::value, "ezWindow doesn't use com interface as window handle. Need accessiable UWP ICoreWindow for ezWindowsHolographicSpace!");

  if (!m_pHolographicSpaceStatics)
    return EZ_FAILURE;
  
  ComPtr<ABI::Windows::UI::Core::ICoreWindow> pCoreWindow;
  EZ_HRESULT_TO_FAILURE_LOG(ComPtr<IUnknown>(window.GetNativeWindowHandle()).As(&pCoreWindow));

  EZ_HRESULT_TO_FAILURE_LOG(m_pHolographicSpaceStatics->CreateForCoreWindow(pCoreWindow.Get(), &m_pHolographicSpace));

  ezLog::Info("Initialized new holographic space for core window!");

  return EZ_SUCCESS;
}

/*

// TODO: Evaluate if and how to expose this.
// WinRT api has a bit of weirdness here:
// "If IsAvailable is false because the user has not yet set up their holographic headset, calling CreateForCoreWindow anyway will guide them through the setup flow."

bool ezWindowsHolographicSpace::IsSupported() const
{
  if (!m_pHolographicSpaceStatics)
    return false;

  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicSpaceStatics2> pStatics2;
  if (FAILED(m_pHolographicSpaceStatics.As(&pStatics2)))
  {
    // If we have not access to statics to we're running pre Creators Update windows!
    // In this case the support is determined by the device type.

  }
  else
  {
    boolean available = FALSE;
    pStatics2->get_IsAvailable(&available);
    return available == TRUE;
  }
}

*/

bool ezWindowsHolographicSpace::IsAvailable() const
{
  if (!m_pHolographicSpaceStatics)
    return false;

  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicSpaceStatics2> pStatics2;
  if (FAILED(m_pHolographicSpaceStatics.As(&pStatics2)))
  {
    // If we have not access to statics to we're running pre Creators Update windows!
    // In this case a headset is exaclty then available if we're on hololens!

    ComPtr<ABI::Windows::System::Profile::IAnalyticsInfoStatics> pAnalyticsStatics;
    if (FAILED(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_System_Profile_AnalyticsInfo).Get(), &pAnalyticsStatics)))
      return false;

    ComPtr<ABI::Windows::System::Profile::IAnalyticsVersionInfo> pAnalyticsVersionInfo;
    if (FAILED(pAnalyticsStatics->get_VersionInfo(&pAnalyticsVersionInfo)))
      return false;

    HString deviceFamily;
    if (FAILED(pAnalyticsVersionInfo->get_DeviceFamily(deviceFamily.GetAddressOf())))
      return false;
    
    return ezStringUtils::IsEqual(ezStringUtf8(deviceFamily).GetData(), "Hololens");
  }
  else
  {
    boolean available = FALSE;
    pStatics2->get_IsAvailable(&available);
    return available == TRUE;
  }
}

EZ_STATICLINK_FILE(WindowsMixedReality, WindowsMixedReality_WindowsHolographicSpace);

