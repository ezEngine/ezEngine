#include <PCH.h>
#include <WindowsMixedReality/HolographicDX11Device.h>
#include <WindowsMixedReality/HolographicSpace.h>

#include <d3d11.h>
#include <dxgi1_4.h>
#pragma warning (push)
#pragma warning (disable: 4467) // warning C4467: usage of ATL attributes is deprecated
#include <windows.graphics.directx.direct3d11.interop.h>
#pragma warning (pop)
#include <windows.graphics.holographic.h>

ezGALHolographicDeviceDX11::ezGALHolographicDeviceDX11(const ezGALDeviceCreationDescription& Description)
  : ezGALDeviceDX11(Description)
{
}

ezGALHolographicDeviceDX11::~ezGALHolographicDeviceDX11()
{
}

ezResult ezGALHolographicDeviceDX11::InitPlatform()
{
  EZ_LOG_BLOCK("ezGALHolographicDeviceDX11::InitPlatform");

  auto pHolographicSpace = ezWindowsHolographicSpace::GetSingleton()->GetInternalHolographicSpace();
  if (!pHolographicSpace)
  {
    ezLog::Error("Can't create holographic DX11 device since there is no holographic space.");
    return EZ_FAILURE;
  }

  // Find out which DXGI factory should be used for DX11 device creation.
  IDXGIAdapter3* pDXGIAdapter = nullptr;
  {
    ABI::Windows::Graphics::Holographic::HolographicAdapterId adapterID;
    EZ_HRESULT_TO_FAILURE_LOG(pHolographicSpace->get_PrimaryAdapterId(&adapterID));
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

  // Enable BGRA support.
  // Without this, it won't be possible to set the device to a holographic space if one is around.
  //
  // Note that it is not entirely clear why this error comes up. The error message is just:
  // "Error: Call 'm_pHolographicSpace->SetDirect3D11Device(m_pDX11InteropDevice.Get())' failed with: The parameter is incorrect."
  // The documentation only mentions that D3D11_CREATE_DEVICE_BGRA_SUPPORT is necessary for Direct2D interop (which we're not relying on as of writing).
  DWORD dwFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

  // Proceed with normal device startup.
  if (ezGALDeviceDX11::InitPlatform(dwFlags, pDXGIAdapter).Failed())
    return EZ_FAILURE;


  // Create interop device.
  {
    // Retrieve WinRT DX11 device pointer.
    ComPtr<ABI::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice> pDX11DeviceWinRT;
    {
      ComPtr<ID3D11Device> pDX11DeviceWinAPI = GetDXDevice();

      // Acquire the DXGI interface for the Direct3D device.
      ComPtr<IDXGIDevice3> pDxgiDevice;
      EZ_HRESULT_TO_FAILURE_LOG(pDX11DeviceWinAPI.As(&pDxgiDevice));

      // Wrap the native device using a WinRT interop object.
      EZ_HRESULT_TO_FAILURE_LOG(CreateDirect3D11DeviceFromDXGIDevice(pDxgiDevice.Get(), &m_pDX11InteropDevice));
    }
  }

  // Inform holographic space.
  EZ_HRESULT_TO_FAILURE_LOG(pHolographicSpace->SetDirect3D11Device(m_pDX11InteropDevice.Get()));

  ezLog::Success("Successfully created DX11 device and set to holographic space!");

  return EZ_SUCCESS;
}

ezResult ezGALHolographicDeviceDX11::ShutdownPlatform()
{
  auto pHolographicSpace = ezWindowsHolographicSpace::GetSingleton()->GetInternalHolographicSpace();
  if (!pHolographicSpace)
  {
    ezLog::Error("Can't destroy holographic DX11 device since there is no holographic space.");
    return EZ_FAILURE;
  }

  pHolographicSpace->SetDirect3D11Device(nullptr);
  m_pDX11InteropDevice.Reset();

  return ShutdownPlatform();
}

ezGALSwapChain* ezGALHolographicDeviceDX11::CreateSwapChainPlatform(const ezGALSwapChainCreationDescription& Description)
{
  // Todo: Create mock swap chain? Or create swapchain per camera?
  return nullptr;
}

void ezGALHolographicDeviceDX11::PresentPlatform(ezGALSwapChain* pSwapChain)
{
  // Todo: Present holographic frame.
}

void ezGALHolographicDeviceDX11::BeginFramePlatform()
{
  // Todo: Create holographic frame.
}

void ezGALHolographicDeviceDX11::EndFramePlatform()
{
  // Todo: Destroy old holographic frame.
}
