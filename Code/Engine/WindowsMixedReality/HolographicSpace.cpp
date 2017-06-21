#include <PCH.h>
#include <WindowsMixedReality/HolographicSpace.h>
#include <WindowsMixedReality/HolographicLocationService.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererDX11/Device/DeviceDX11.h>

#include <d3d11.h>
#include <dxgi1_4.h>
#pragma warning (push)
#pragma warning (disable: 4467) // warning C4467: usage of ATL attributes is deprecated
#include <windows.graphics.directx.direct3d11.interop.h>
#pragma warning (pop)
#include <windows.graphics.directx.direct3d11.h>
#include <windows.graphics.holographic.h>
#include <windows.system.profile.h>

#include <wrl/event.h>

EZ_IMPLEMENT_SINGLETON(ezWindowsHolographicSpace);

EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, WindowsHolographicSpace)

ON_CORE_STARTUP
{
  ezWindowsHolographicSpace* holoSpace = EZ_DEFAULT_NEW(ezWindowsHolographicSpace);
  holoSpace->InitForMainCoreWindow();
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
  DeInit();
}

ezResult ezWindowsHolographicSpace::InitForMainCoreWindow()
{
  if (!m_pHolographicSpaceStatics)
    return EZ_FAILURE;

  DeInit();

  // Create holographic space from core window
  {
    ComPtr<ABI::Windows::UI::Core::ICoreWindow> pCoreWindow;
    {
      ComPtr<ABI::Windows::ApplicationModel::Core::ICoreImmersiveApplication> application;
      EZ_HRESULT_TO_FAILURE(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(), &application));

      ComPtr<ABI::Windows::ApplicationModel::Core::ICoreApplicationView> mainView;
      EZ_HRESULT_TO_FAILURE(application->get_MainView(&mainView));

      EZ_HRESULT_TO_FAILURE(mainView->get_CoreWindow(&pCoreWindow));
    }

    EZ_HRESULT_TO_FAILURE_LOG(m_pHolographicSpaceStatics->CreateForCoreWindow(pCoreWindow.Get(), &m_pHolographicSpace));
  }

  // Find out which DXGI factory should be used for DX11 device creation.
  {
    ABI::Windows::Graphics::Holographic::HolographicAdapterId adapterID;
    EZ_HRESULT_TO_FAILURE_LOG(m_pHolographicSpace->get_PrimaryAdapterId(&adapterID));
    LUID id;
    id.HighPart = adapterID.HighPart;
    id.LowPart = adapterID.LowPart;
    
    // When a primary adapter ID is given to the app, the app should find the corresponding DXGI adapter and use it to create Direct3D devices
    // and device contexts. Otherwise, there is no restriction on the DXGI adapter the app can use.
    if (adapterID.HighPart != 0 && adapterID.LowPart != 0)
    {
      UINT createFlags = 0;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      createFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
      // Create the DXGI factory 4.
      ComPtr<IDXGIFactory1> pDxgiFactory;
      EZ_HRESULT_TO_FAILURE_LOG(CreateDXGIFactory2(createFlags, IID_PPV_ARGS(pDxgiFactory.GetAddressOf())));
      ComPtr<IDXGIFactory4> pDxgiFactory4;
      EZ_HRESULT_TO_FAILURE_LOG(pDxgiFactory.As(&pDxgiFactory4));

      // Retrieve the adapter specified by the holographic space.
      Microsoft::WRL::ComPtr<IDXGIAdapter3> m_pDxgiAdapter;
      EZ_HRESULT_TO_FAILURE_LOG(pDxgiFactory4->EnumAdapterByLuid(id, IID_PPV_ARGS(m_pDxgiAdapter.GetAddressOf())));
    }
  }

  
  // Register to camera added/removed
  {
    using OnCameraAdded = __FITypedEventHandler_2_Windows__CGraphics__CHolographic__CHolographicSpace_Windows__CGraphics__CHolographic__CHolographicSpaceCameraAddedEventArgs;
    EZ_HRESULT_TO_FAILURE_LOG(m_pHolographicSpace->add_CameraAdded(Callback<OnCameraAdded>(this, &ezWindowsHolographicSpace::OnCameraAdded).Get(), &m_eventRegistrationOnCameraAdded));

    using OnCameraRemoved = __FITypedEventHandler_2_Windows__CGraphics__CHolographic__CHolographicSpace_Windows__CGraphics__CHolographic__CHolographicSpaceCameraRemovedEventArgs;
    EZ_HRESULT_TO_FAILURE_LOG(m_pHolographicSpace->add_CameraRemoved(Callback<OnCameraRemoved>(this, &ezWindowsHolographicSpace::OnCameraRemoved).Get(), &m_eventRegistrationOnCameraRemoved));
  }

  // Setup locator
  {
    ComPtr<ABI::Windows::Perception::Spatial::ISpatialLocatorStatics> pSpatialLocatorStatics;
    EZ_HRESULT_TO_FAILURE_LOG(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_Perception_Spatial_SpatialLocator).Get(), &pSpatialLocatorStatics));

    ComPtr<ABI::Windows::Perception::Spatial::ISpatialLocator> pDefaultSpatialLocator;
    EZ_HRESULT_TO_FAILURE_LOG(pSpatialLocatorStatics->GetDefault(&pDefaultSpatialLocator));

    m_pDefaultLocationService = EZ_DEFAULT_NEW(ezWindowsHolographicLocationService, pDefaultSpatialLocator);
  }

  ezLog::Info("Initialized new holographic space for main window!");

  return EZ_SUCCESS;
}

void ezWindowsHolographicSpace::DeInit()
{
  if (!m_pHolographicSpaceStatics)
    return;

  m_pDefaultLocationService.Reset();

  if (m_pHolographicSpace)
  {
    m_pHolographicSpace->remove_CameraAdded(m_eventRegistrationOnCameraAdded);
    m_pHolographicSpace->remove_CameraRemoved(m_eventRegistrationOnCameraRemoved);
    m_pHolographicSpace = nullptr;
  }
}

ezResult ezWindowsHolographicSpace::SetDX11Device()
{
  // Retrieve WinRT DX11 device pointer.
  ComPtr<ABI::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice> pDX11DeviceWinRT;
  {
    auto pDX11DeviceEz = static_cast<ezGALDeviceDX11*>(ezGALDevice::GetDefaultDevice());
    if (!pDX11DeviceEz)
    {
      ezLog::Error("Windows holographic space requires the default device to be a DX11 device!");
      return EZ_FAILURE;
    }

    ComPtr<ID3D11Device> pDX11DeviceWinAPI = pDX11DeviceEz->GetDXDevice();

    // Acquire the DXGI interface for the Direct3D device.
    ComPtr<IDXGIDevice3> pDxgiDevice;
    EZ_HRESULT_TO_FAILURE_LOG(pDX11DeviceWinAPI.As(&pDxgiDevice));

    // Wrap the native device using a WinRT interop object.
    EZ_HRESULT_TO_FAILURE_LOG(CreateDirect3D11DeviceFromDXGIDevice(pDxgiDevice.Get(), &m_pDX11InteropDevice));
  }

  EZ_HRESULT_TO_FAILURE_LOG(m_pHolographicSpace->SetDirect3D11Device(m_pDX11InteropDevice.Get()));

  ezLog::Info("Set DX11 device to holographic space!");

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

#if EZ_WINRT_SDK_VERSION > EZ_WIN_SDK_VERSION_10_RS1
  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicSpaceStatics2> pStatics2;
  if (FAILED(m_pHolographicSpaceStatics.As(&pStatics2)))
  {
#endif
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
    
    return ezStringUtils::IsEqual(ezStringUtf8(deviceFamily).GetData(), "Windows.Holographic");
#if EZ_WINRT_SDK_VERSION > EZ_WIN_SDK_VERSION_10_RS1
  }
  else
  {
    boolean available = FALSE;
    pStatics2->get_IsAvailable(&available);
    return available == TRUE;
  }
#endif
}

HRESULT ezWindowsHolographicSpace::OnCameraAdded(ABI::Windows::Graphics::Holographic::IHolographicSpace* holographicSpace, ABI::Windows::Graphics::Holographic::IHolographicSpaceCameraAddedEventArgs* args)
{
  // todo

  return S_OK;
}

HRESULT ezWindowsHolographicSpace::OnCameraRemoved(ABI::Windows::Graphics::Holographic::IHolographicSpace* holographicSpace, ABI::Windows::Graphics::Holographic::IHolographicSpaceCameraRemovedEventArgs* args)
{

  // Holographic frame predictions will not include any information about this camera until
  // the deferral is completed.
  //deferral->Complete();

  return S_OK;
}

EZ_STATICLINK_FILE(WindowsMixedReality, WindowsMixedReality_WindowsHolographicSpace);

