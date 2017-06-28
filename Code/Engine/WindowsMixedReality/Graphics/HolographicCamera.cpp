#include <PCH.h>
#include <WindowsMixedReality/Graphics/HolographicCamera.h>
#include <WindowsMixedReality/Graphics/HolographicSwapChainDX11.h>

#include <RendererFoundation/Device/Device.h>
#include <windows.graphics.holographic.h>

ezWindowsHolographicCamera::ezWindowsHolographicCamera(const ComPtr<ABI::Windows::Graphics::Holographic::IHolographicCamera> pHolographicCamera)
  : m_pHolographicCamera(pHolographicCamera)
{
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice(); // This needs to be a holographic device.

  // Create (holographic) swap chain for this camera.
  // Desc doesn't really matter since there isn't any *actual* swapchain. The holographic swap chain just represents the backbuffer of this camera.
  ezGALSwapChainCreationDescription desc;
  desc.m_pWindow = &ezGALHolographicSwapChainDX11::s_mockWindow;
  desc.m_SampleCount = ezGALMSAASampleCount::None;
  desc.m_BackBufferFormat = ezGALResourceFormat::Invalid;
  desc.m_bAllowScreenshots = false;
  desc.m_bDoubleBuffered = false;
  desc.m_bVerticalSynchronization = false;
  m_associatedSwapChain = pDevice->CreateSwapChain(desc);

  // Make this the primary swap chain if we're the first.
  if (pDevice->GetPrimarySwapChain().IsInvalidated())
    pDevice->SetPrimarySwapChain(m_associatedSwapChain);
}

ezWindowsHolographicCamera::~ezWindowsHolographicCamera()
{
  ezGALDevice::GetDefaultDevice()->DestroySwapChain(m_associatedSwapChain);
}

ezUInt32 ezWindowsHolographicCamera::GetId() const
{
  UINT id;
  if (FAILED(m_pHolographicCamera->get_Id(&id)))
  {
    ezLog::Error("Failed to retrieve holographic camera's id.");
    return ezMath::BasicType<ezUInt32>::MaxValue();
  }

  return id;
}

ezRectU32 ezWindowsHolographicCamera::GetBackBufferSize() const
{
  ABI::Windows::Foundation::Size size;
  if (FAILED(m_pHolographicCamera->get_RenderTargetSize(&size)))
  {
    ezLog::Error("Failed to retrieve holographic camera backbuffer size.");
    return ezRectU32(0, 0);
  }

  return ezRectU32(static_cast<ezUInt32>(size.Width), static_cast<ezUInt32>(size.Height));
}

bool ezWindowsHolographicCamera::IsStereoscopic() const
{
  boolean isStereo = FALSE;
  if (FAILED(m_pHolographicCamera->get_IsStereo(&isStereo)))
  {
    ezLog::Error("Failed to check query holographic camera is stereo.");
    return false;
  }

  return isStereo == TRUE;
}

HRESULT ezWindowsHolographicCamera::UpdatePose(ABI::Windows::Graphics::Holographic::IHolographicCameraPose* pPose,
                                               ABI::Windows::Graphics::Holographic::IHolographicCameraRenderingParameters* pRenderingParameters)
{
  // Update projection.
  {
    ABI::Windows::Graphics::Holographic::HolographicStereoTransform stereoTransform;
    EZ_HRESULT_TO_FAILURE_LOG(pPose->get_ProjectionTransform(&stereoTransform));

    memcpy_s(&m_projectionMatrices[0], sizeof(m_projectionMatrices[0]), &stereoTransform.Left, sizeof(stereoTransform.Left));

    if (IsStereoscopic())
      memcpy_s(&m_projectionMatrices[1], sizeof(m_projectionMatrices[1]), &stereoTransform.Right, sizeof(stereoTransform.Right));
    else
      m_projectionMatrices[1] = m_projectionMatrices[0];
  }

  // Update viewport
  {
    ABI::Windows::Foundation::Rect viewport;
    EZ_HRESULT_TO_FAILURE_LOG(pPose->get_Viewport(&viewport));
    m_viewport.x = viewport.X;
    m_viewport.y = viewport.Y;
    m_viewport.height = viewport.Height;
    m_viewport.width = viewport.Width;
  }

  // Update view transforms.
  // TODO ?? Needs to be relative to coordinate system!
  // Need to do on demand? Store pose?

  // Ensure swap chain is up to date.
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    ezGALHolographicSwapChainDX11* pSwapChain = const_cast<ezGALHolographicSwapChainDX11*>(static_cast<const ezGALHolographicSwapChainDX11*>(pDevice->GetSwapChain(m_associatedSwapChain)));
    pSwapChain->EnsureBackBufferResources(pDevice, pRenderingParameters);
  }

  return S_OK;
}


EZ_STATICLINK_FILE(WindowsMixedReality, WindowsMixedReality_Graphics_HolographicCamera);

