#include <PCH.h>
#include <WindowsMixedReality/HolographicSpace.h>
#include <WindowsMixedReality/HolographicLocationService.h>
#include <WindowsMixedReality/Graphics/HolographicCamera.h>

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
  EZ_LOG_BLOCK("ezWindowsHolographicSpace::InitForMainCoreWindow");

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

  for (auto pCamera : m_cameras)
    EZ_DEFAULT_DELETE(pCamera);
  m_cameras.Clear();

  m_pDefaultLocationService.Reset();

  if (m_pHolographicSpace)
  {
    m_pHolographicSpace->remove_CameraAdded(m_eventRegistrationOnCameraAdded);
    m_pHolographicSpace->remove_CameraRemoved(m_eventRegistrationOnCameraRemoved);
    m_pHolographicSpace = nullptr;
  }
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

ComPtr<ABI::Windows::Graphics::Holographic::IHolographicFrame> ezWindowsHolographicSpace::StartNewHolographicFrame()
{
  EZ_ASSERT_DEBUG(m_pHolographicSpace, "There is no holographic space.");

  // Handle added/removed holographic cameras.
  ProcessAddedRemovedCameras();

  // Create holographic frame.
  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicFrame> pHolographicFrame;
  HRESULT result = m_pHolographicSpace->CreateNextFrame(pHolographicFrame.GetAddressOf());
  if (FAILED(result))
  {
    ezLog::Error("Failed to create holographic frame: '{0}'", ezHRESULTtoString(result));
    return nullptr;
  }

  // Use it to update all our cameras.
  UpdateCameraPoses(pHolographicFrame);


  return std::move(pHolographicFrame);
}

void ezWindowsHolographicSpace::ProcessAddedRemovedCameras()
{
  ezLock<ezMutex> lock(m_cameraMutex);

  // Process removals.
  for (const auto& pCamera : m_pendingCameraRemovals)
  {
    for (ezUInt32 i = 0; i < m_cameras.GetCount(); ++i)
    {
      if (m_cameras[i]->GetInternalHolographicCamera() == pCamera.Get())
      {
        EZ_DEFAULT_DELETE(m_cameras[i]);
        m_cameras.RemoveAt(i);
        break;
      }
    }
  }
  m_pendingCameraRemovals.Clear();

  // Process additions.
  for (const auto& cameraAddition : m_pendingCameraAdditions)
  {
    m_cameras.PushBack(EZ_DEFAULT_NEW(ezWindowsHolographicCamera, cameraAddition.m_pCamera));
    cameraAddition.m_pDeferral->Complete();
  }
  m_pendingCameraAdditions.Clear();
}

ezResult ezWindowsHolographicSpace::UpdateCameraPoses(const ComPtr<ABI::Windows::Graphics::Holographic::IHolographicFrame>& pHolographicFrame)
{
  // Get prediction.
  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicFramePrediction> pPrediction;
  EZ_HRESULT_TO_FAILURE(pHolographicFrame->get_CurrentPrediction(pPrediction.GetAddressOf()));

  // Get camera poses.
  ComPtr<ABI::Windows::Foundation::Collections::IVectorView<ABI::Windows::Graphics::Holographic::HolographicCameraPose*>> pCameraPoses;
  EZ_HRESULT_TO_FAILURE(pPrediction->get_CameraPoses(&pCameraPoses));

  // TODO
  // .........

  return EZ_SUCCESS;
}

HRESULT ezWindowsHolographicSpace::OnCameraAdded(ABI::Windows::Graphics::Holographic::IHolographicSpace* holographicSpace, ABI::Windows::Graphics::Holographic::IHolographicSpaceCameraAddedEventArgs* args)
{
  ezLock<ezMutex> lock(m_cameraMutex);

  auto& pendingAddition = m_pendingCameraAdditions.ExpandAndGetRef();
  EZ_HRESULT_TO_FAILURE_LOG(args->get_Camera(pendingAddition.m_pCamera.GetAddressOf()));
  EZ_HRESULT_TO_FAILURE_LOG(args->GetDeferral(pendingAddition.m_pDeferral.GetAddressOf()));

  return S_OK;
}

HRESULT ezWindowsHolographicSpace::OnCameraRemoved(ABI::Windows::Graphics::Holographic::IHolographicSpace* holographicSpace, ABI::Windows::Graphics::Holographic::IHolographicSpaceCameraRemovedEventArgs* args)
{
  ezLock<ezMutex> lock(m_cameraMutex);

  ComPtr<ABI::Windows::Graphics::Holographic::IHolographicCamera> pCamera;
  EZ_HRESULT_TO_FAILURE_LOG(args->get_Camera(pCamera.GetAddressOf()));
  m_pendingCameraRemovals.PushBack(std::move(pCamera));

  return S_OK;
}

EZ_STATICLINK_FILE(WindowsMixedReality, WindowsMixedReality_WindowsHolographicSpace);

