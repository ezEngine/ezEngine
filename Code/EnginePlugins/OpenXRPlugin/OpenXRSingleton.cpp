#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <GameEngine/GameApplication/GameApplication.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRHandTracking.h>
#include <OpenXRPlugin/OpenXRInputDevice.h>
#include <OpenXRPlugin/OpenXRRemoting.h>
#include <OpenXRPlugin/OpenXRSingleton.h>
#include <OpenXRPlugin/OpenXRSpatialAnchors.h>

#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/World/World.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <GameEngine/XR/XRWindow.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <RendererDX11/Device/DeviceDX11.h>
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
#  include <winrt/Windows.Graphics.Holographic.h>
#  include <winrt/Windows.UI.Core.h>
#  include <winrt/base.h>
#endif

#include <vector>

EZ_CHECK_AT_COMPILETIME(ezGALMSAASampleCount::None == 1);
EZ_CHECK_AT_COMPILETIME(ezGALMSAASampleCount::TwoSamples == 2);
EZ_CHECK_AT_COMPILETIME(ezGALMSAASampleCount::FourSamples == 4);
EZ_CHECK_AT_COMPILETIME(ezGALMSAASampleCount::EightSamples == 8);

EZ_IMPLEMENT_SINGLETON(ezOpenXR);

static ezOpenXR g_OpenXRSingleton;


ezOpenXR::ezOpenXR()
  : m_SingletonRegistrar(this)
{
#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  m_remoting = EZ_DEFAULT_NEW(ezOpenXRRemoting, this);
#endif
}

ezOpenXR::~ezOpenXR() {}

bool ezOpenXR::IsHmdPresent() const
{
  XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
  systemInfo.formFactor = XrFormFactor::XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  uint64_t systemId = XR_NULL_SYSTEM_ID;
  XrResult res = xrGetSystem(m_instance, &systemInfo, &systemId);

  return res == XrResult::XR_SUCCESS;
}

XrResult ezOpenXR::SelectExtensions(ezHybridArray<const char*, 6>& extensions)
{
  // Fetch the list of extensions supported by the runtime.
  ezUInt32 extensionCount;
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr));
  std::vector<XrExtensionProperties> extensionProperties(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensionProperties.data()));

  // Add a specific extension to the list of extensions to be enabled, if it is supported.
  auto AddExtIfSupported = [&](const char* extensionName, bool& enableFlag) -> XrResult {
    auto it = std::find_if(begin(extensionProperties), end(extensionProperties), [&](const XrExtensionProperties& prop) { return ezStringUtils::IsEqual(prop.extensionName, extensionName); });
    if (it != end(extensionProperties))
    {
      extensions.PushBack(extensionName);
      enableFlag = true;
      return XR_SUCCESS;
    }
    enableFlag = false;
    return XR_ERROR_EXTENSION_NOT_PRESENT;
  };

  // D3D11 extension is required so check that it was added.
  XR_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(XR_KHR_D3D11_ENABLE_EXTENSION_NAME, m_extensions.m_bD3D11));

  AddExtIfSupported(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME, m_extensions.m_bDepthComposition);
  AddExtIfSupported(XR_MSFT_UNBOUNDED_REFERENCE_SPACE_EXTENSION_NAME, m_extensions.m_bUnboundedReferenceSpace);
  AddExtIfSupported(XR_MSFT_SPATIAL_ANCHOR_EXTENSION_NAME, m_extensions.m_bSpatialAnchor);
  AddExtIfSupported(XR_EXT_HAND_TRACKING_EXTENSION_NAME, m_extensions.m_bHandTracking);
  AddExtIfSupported(XR_MSFT_HAND_INTERACTION_EXTENSION_NAME, m_extensions.m_bHandInteraction);
  AddExtIfSupported(XR_MSFT_HAND_TRACKING_MESH_EXTENSION_NAME, m_extensions.m_bHandTrackingMesh);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  AddExtIfSupported(XR_MSFT_HOLOGRAPHIC_WINDOW_ATTACHMENT_EXTENSION_NAME, m_extensions.m_bHolographicWindowAttachment);
#endif

#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  AddExtIfSupported(XR_MSFT_HOLOGRAPHIC_REMOTING_EXTENSION_NAME, m_extensions.m_bRemoting);
#endif

#ifdef BUILDSYSTEM_ENABLE_OPENXR_PREVIEW_SUPPORT
#endif
  return XR_SUCCESS;
}

#define EZ_GET_INSTANCE_PROC_ADDR(name) (void)xrGetInstanceProcAddr(m_instance, #name, reinterpret_cast<PFN_xrVoidFunction*>(&m_extensions.pfn_##name));

ezResult ezOpenXR::Initialize()
{
  if (m_instance != XR_NULL_HANDLE)
    return EZ_SUCCESS;

  // Build out the extensions to enable. Some extensions are required and some are optional.
  ezHybridArray<const char*, 6> enabledExtensions;
  if (SelectExtensions(enabledExtensions) != XR_SUCCESS)
    return EZ_FAILURE;

  // Create the instance with desired extensions.
  XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
  createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.GetCount();
  createInfo.enabledExtensionNames = enabledExtensions.GetData();

  ezStringUtils::Copy(createInfo.applicationInfo.applicationName, EZ_ARRAY_SIZE(createInfo.applicationInfo.applicationName), ezApplication::GetApplicationInstance()->GetApplicationName());
  ezStringUtils::Copy(createInfo.applicationInfo.engineName, EZ_ARRAY_SIZE(createInfo.applicationInfo.engineName), "ezEngine");
  createInfo.applicationInfo.engineVersion = 1;
  createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
  createInfo.applicationInfo.applicationVersion = 1;
  XrResult res = xrCreateInstance(&createInfo, &m_instance);
  if (res != XR_SUCCESS)
  {
    ezLog::Error("InitSystem xrCreateInstance failed: {}", res);
    Deinitialize();
    return EZ_FAILURE;
  }
  XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
  res = xrGetInstanceProperties(m_instance, &instanceProperties);
  if (res != XR_SUCCESS)
  {
    ezLog::Error("InitSystem xrGetInstanceProperties failed: {}", res);
    Deinitialize();
    return EZ_FAILURE;
  }
  ezStringBuilder sTemp;
  m_Info.m_sDeviceDriver = ezConversionUtils::ToString(instanceProperties.runtimeVersion, sTemp);

  EZ_GET_INSTANCE_PROC_ADDR(xrGetD3D11GraphicsRequirementsKHR);

  if (m_extensions.m_bSpatialAnchor)
  {
    EZ_GET_INSTANCE_PROC_ADDR(xrCreateSpatialAnchorMSFT);
    EZ_GET_INSTANCE_PROC_ADDR(xrCreateSpatialAnchorSpaceMSFT);
    EZ_GET_INSTANCE_PROC_ADDR(xrDestroySpatialAnchorMSFT);
  }

  if (m_extensions.m_bHandTracking)
  {
    EZ_GET_INSTANCE_PROC_ADDR(xrCreateHandTrackerEXT);
    EZ_GET_INSTANCE_PROC_ADDR(xrDestroyHandTrackerEXT);
    EZ_GET_INSTANCE_PROC_ADDR(xrLocateHandJointsEXT);
  }

  if (m_extensions.m_bHandTrackingMesh)
  {
    EZ_GET_INSTANCE_PROC_ADDR(xrCreateHandMeshSpaceMSFT);
    EZ_GET_INSTANCE_PROC_ADDR(xrUpdateHandMeshMSFT);
  }

#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  if (m_extensions.m_bRemoting)
  {
    EZ_GET_INSTANCE_PROC_ADDR(xrRemotingSetContextPropertiesMSFT);
    EZ_GET_INSTANCE_PROC_ADDR(xrRemotingConnectMSFT);
    EZ_GET_INSTANCE_PROC_ADDR(xrRemotingDisconnectMSFT);
    EZ_GET_INSTANCE_PROC_ADDR(xrRemotingGetConnectionStateMSFT);
  }
#endif

  m_Input = EZ_DEFAULT_NEW(ezOpenXRInputDevice, this);

  m_executionEventsId = ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(ezMakeDelegate(&ezOpenXR::GameApplicationEventHandler, this));

  res = InitSystem();
  if (res != XR_SUCCESS)
  {
    ezLog::Error("InitSystem failed: {}", res);
    Deinitialize();
    return EZ_FAILURE;
  }

  ezLog::Success("OpenXR {0} v{1} initialized successfully.", instanceProperties.runtimeName, instanceProperties.runtimeVersion);
  return EZ_SUCCESS;
}

void ezOpenXR::Deinitialize()
{
#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  m_remoting->Disconnect().IgnoreResult();
#endif

  if (m_executionEventsId != 0)
  {
    ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(m_executionEventsId);
  }

  DeinitSwapChain();
  DeinitSession();
  DeinitSystem();

  m_Input = nullptr;

  if (m_instance)
  {
    xrDestroyInstance(m_instance);
    m_instance = XR_NULL_HANDLE;
  }
}

bool ezOpenXR::IsInitialized() const
{
  return m_instance != XR_NULL_HANDLE;
}

const ezHMDInfo& ezOpenXR::GetHmdInfo() const
{
  EZ_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");
  return m_Info;
}

ezXRInputDevice& ezOpenXR::GetXRInput() const
{
  return *(m_Input.Borrow());
}

ezUniquePtr<ezActor> ezOpenXR::CreateActor(ezView* pView, ezGALMSAASampleCount::Enum msaaCount, ezUniquePtr<ezWindowBase> companionWindow, ezUniquePtr<ezWindowOutputTargetBase> companionWindowOutput)
{
  EZ_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  XrResult res = InitSession();
  if (res != XrResult::XR_SUCCESS)
  {
    ezLog::Error("InitSession failed: {}", res);
    return {};
  }

  res = InitSwapChain(msaaCount);
  if (res != XrResult::XR_SUCCESS)
  {
    DeinitSession();
    ezLog::Error("InitSwapChain failed: {}", res);
    return {};
  }

  {
    EZ_ASSERT_DEV(pView->GetCamera() != nullptr, "The provided view requires a camera to be set.");
    SetHMDCamera(pView->GetCamera());
  }

  ezUniquePtr<ezActor> pActor = EZ_DEFAULT_NEW(ezActor, "OpenXR", this);

  EZ_ASSERT_DEV((companionWindow != nullptr) == (companionWindowOutput != nullptr), "Both companionWindow and companionWindowOutput must either be null or valid.");
  EZ_ASSERT_DEV(companionWindow == nullptr || SupportsCompanionView(), "If a companionWindow is set, SupportsCompanionView() must be true.");

  ezUniquePtr<ezActorPluginWindowXR> pActorPlugin = EZ_DEFAULT_NEW(ezActorPluginWindowXR, this, std::move(companionWindow), std::move(companionWindowOutput));
  pActor->AddPlugin(std::move(pActorPlugin));

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  m_hView = pView->GetHandle();
  m_pWorld = pView->GetWorld();
  EZ_ASSERT_DEV(m_pWorld != nullptr, "");

  m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(m_hColorRT));
  m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(m_hDepthRT));

  pView->SetRenderTargetSetup(m_RenderTargetSetup);

  pView->SetViewport(ezRectFloat((float)m_Info.m_vEyeRenderTargetSize.width, (float)m_Info.m_vEyeRenderTargetSize.height));

  return std::move(pActor);
}

void ezOpenXR::OnActorDestroyed()
{
  if (m_hView.IsInvalidated())
    return;

  m_pWorld = nullptr;
  SetHMDCamera(nullptr);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezRenderWorld::RemoveMainView(m_hView);
  m_hView.Invalidate();
  m_RenderTargetSetup.DestroyAllAttachedViews();

  DeinitSwapChain();
  DeinitSession();
}

bool ezOpenXR::SupportsCompanionView()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  return true;
#else
  // E.g. on UWP OpenXR creates its own main window and other resources that conflict with our window.
  // Thus we must prevent the creation of a companion view or OpenXR crashes.
  return false;
#endif
}

XrSpace ezOpenXR::GetBaseSpace() const
{
  return m_StageSpace == ezXRStageSpace::Standing ? m_sceneSpace : m_localSpace;
}

XrResult ezOpenXR::InitSystem()
{
  EZ_ASSERT_DEV(m_systemId == XR_NULL_SYSTEM_ID, "OpenXR actor already exists.");
  XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
  systemInfo.formFactor = XrFormFactor::XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  XR_SUCCEED_OR_CLEANUP_LOG(xrGetSystem(m_instance, &systemInfo, &m_systemId), DeinitSystem);

  return XrResult::XR_SUCCESS;
}

void ezOpenXR::DeinitSystem()
{
  m_systemId = XR_NULL_SYSTEM_ID;
}

XrResult ezOpenXR::InitSession()
{
  EZ_ASSERT_DEV(m_session == XR_NULL_HANDLE, "");

  ezUInt32 count;
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, m_primaryViewConfigurationType, 0, &count, nullptr), DeinitSystem);

  ezHybridArray<XrEnvironmentBlendMode, 4> environmentBlendModes;
  environmentBlendModes.SetCount(count);
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateEnvironmentBlendModes(m_instance, m_systemId, m_primaryViewConfigurationType, count, &count, environmentBlendModes.GetData()), DeinitSession);

  // Select preferred blend mode.
  m_blendMode = environmentBlendModes[0];

  XR_SUCCEED_OR_CLEANUP_LOG(InitGraphicsPlugin(), DeinitSession);

  XrSessionCreateInfo sessionCreateInfo{XR_TYPE_SESSION_CREATE_INFO};
  sessionCreateInfo.systemId = m_systemId;
  sessionCreateInfo.next = &m_xrGraphicsBindingD3D11;

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  if (m_extensions.m_bHolographicWindowAttachment)
  {
    // Creating a HolographicSpace before activating the CoreWindow to make it a holographic window
    winrt::Windows::UI::Core::CoreWindow window = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
    winrt::Windows::Graphics::Holographic::HolographicSpace holographicSpace = winrt::Windows::Graphics::Holographic::HolographicSpace::CreateForCoreWindow(window);
    window.Activate();

    XrHolographicWindowAttachmentMSFT holographicWindowAttachment{XR_TYPE_HOLOGRAPHIC_WINDOW_ATTACHMENT_MSFT};
    {
      holographicWindowAttachment.next = &m_xrGraphicsBindingD3D11;
      // TODO: The code in this block works around the fact that for some reason the commented out winrt equivalent does not compile although it works in every sample:
      // error C2131: expression did not evaluate to a constant.
      // holographicWindowAttachment.coreWindow = window.as<IUnknown>().get();
      // holographicWindowAttachment.holographicSpace = holographicSpace.as<IUnknown>().get();
      winrt::com_ptr<IUnknown> temp;
      winrt::copy_to_abi(window.as<winrt::Windows::Foundation::IUnknown>(), *temp.put_void());
      holographicWindowAttachment.coreWindow = temp.detach();

      winrt::copy_to_abi(holographicSpace.as<winrt::Windows::Foundation::IUnknown>(), *temp.put_void());
      holographicWindowAttachment.holographicSpace = temp.detach();
    }

    sessionCreateInfo.next = &holographicWindowAttachment;
  }
#endif

  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateSession(m_instance, &sessionCreateInfo, &m_session), DeinitSession);

  XrReferenceSpaceCreateInfo spaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  if (m_extensions.m_bUnboundedReferenceSpace)
  {
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_UNBOUNDED_MSFT;
  }
  else
  {
    spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
  }

  spaceCreateInfo.poseInReferenceSpace = ConvertTransform(ezTransform::IdentityTransform());
  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateReferenceSpace(m_session, &spaceCreateInfo, &m_sceneSpace), DeinitSession);

  spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateReferenceSpace(m_session, &spaceCreateInfo, &m_localSpace), DeinitSession);

  XR_SUCCEED_OR_CLEANUP_LOG(m_Input->CreateActions(m_session, m_sceneSpace), DeinitSession);
  XR_SUCCEED_OR_CLEANUP_LOG(m_Input->AttachSessionActionSets(m_session), DeinitSession);

  m_GALdeviceEventsId = ezGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(ezMakeDelegate(&ezOpenXR::GALDeviceEventHandler, this));

  SetStageSpace(ezXRStageSpace::Standing);
  if (m_extensions.m_bSpatialAnchor)
  {
    m_Anchors = EZ_DEFAULT_NEW(ezOpenXRSpatialAnchors, this);
  }
  if (m_extensions.m_bHandTracking && ezOpenXRHandTracking::IsHandTrackingSupported(this))
  {
    m_HandTracking = EZ_DEFAULT_NEW(ezOpenXRHandTracking, this);
  }
  return XrResult::XR_SUCCESS;
}

void ezOpenXR::DeinitSession()
{
  m_sessionRunning = false;
  m_exitRenderLoop = false;
  m_requestRestart = false;
  m_renderInProgress = false;
  m_sessionState = XR_SESSION_STATE_UNKNOWN;

  m_HandTracking = nullptr;
  m_Anchors = nullptr;
  if (m_GALdeviceEventsId != 0)
  {
    ezGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(m_GALdeviceEventsId);
  }

  if (m_sceneSpace)
  {
    xrDestroySpace(m_sceneSpace);
    m_sceneSpace = XR_NULL_HANDLE;
  }

  if (m_localSpace)
  {
    xrDestroySpace(m_localSpace);
    m_localSpace = XR_NULL_HANDLE;
  }

  m_Input->DestroyActions();

  if (m_session)
  {
    xrDestroySession(m_session);
    m_session = XR_NULL_HANDLE;
  }

  DeinitGraphicsPlugin();
}

XrResult ezOpenXR::InitGraphicsPlugin()
{
  EZ_ASSERT_DEV(m_xrGraphicsBindingD3D11.device == nullptr, "");
  // Hard-coded to d3d
  XrGraphicsRequirementsD3D11KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
  XR_SUCCEED_OR_CLEANUP_LOG(m_extensions.pfn_xrGetD3D11GraphicsRequirementsKHR(m_instance, m_systemId, &graphicsRequirements), DeinitGraphicsPlugin);
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALDeviceDX11* pD3dDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  m_xrGraphicsBindingD3D11.device = pD3dDevice->GetDXDevice();

  return XrResult::XR_SUCCESS;
}

void ezOpenXR::DeinitGraphicsPlugin()
{
  m_xrGraphicsBindingD3D11.device = nullptr;
}

XrResult ezOpenXR::SelectSwapchainFormat(int64_t& colorFormat, int64_t& depthFormat)
{
  uint32_t swapchainFormatCount;
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateSwapchainFormats(m_session, 0, &swapchainFormatCount, nullptr), voidFunction);
  std::vector<int64_t> swapchainFormats(swapchainFormatCount);
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateSwapchainFormats(m_session, (uint32_t)swapchainFormats.size(), &swapchainFormatCount, swapchainFormats.data()), voidFunction);

  // List of supported color swapchain formats, in priority order.
  constexpr DXGI_FORMAT SupportedColorSwapchainFormats[] = {
    DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
    DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
  };

  constexpr DXGI_FORMAT SupportedDepthSwapchainFormats[] = {
    DXGI_FORMAT_D32_FLOAT,
    DXGI_FORMAT_D16_UNORM,
    DXGI_FORMAT_D24_UNORM_S8_UINT,
  };

  auto swapchainFormatIt = std::find_first_of(std::begin(SupportedColorSwapchainFormats), std::end(SupportedColorSwapchainFormats), swapchainFormats.begin(), swapchainFormats.end());
  if (swapchainFormatIt == std::end(SupportedColorSwapchainFormats))
  {
    return XrResult::XR_ERROR_INITIALIZATION_FAILED;
  }
  colorFormat = *swapchainFormatIt;

  if (m_extensions.m_bDepthComposition)
  {
    auto depthSwapchainFormatIt = std::find_first_of(std::begin(SupportedDepthSwapchainFormats), std::end(SupportedDepthSwapchainFormats), swapchainFormats.begin(), swapchainFormats.end());
    if (depthSwapchainFormatIt == std::end(SupportedDepthSwapchainFormats))
    {
      return XrResult::XR_ERROR_INITIALIZATION_FAILED;
    }
    depthFormat = *depthSwapchainFormatIt;
  }
  return XrResult::XR_SUCCESS;
}

XrResult ezOpenXR::CreateSwapchainImages(Swapchain& swapchain, SwapchainType type)
{
  if (type == SwapchainType::Color)
  {
    m_colorSwapChainImagesD3D11.SetCount(swapchain.imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
    swapchain.images = reinterpret_cast<XrSwapchainImageBaseHeader*>(m_colorSwapChainImagesD3D11.GetData());
  }
  else
  {
    m_depthSwapChainImagesD3D11.SetCount(swapchain.imageCount, {XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR});
    swapchain.images = reinterpret_cast<XrSwapchainImageBaseHeader*>(m_depthSwapChainImagesD3D11.GetData());
  }
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateSwapchainImages(swapchain.handle, swapchain.imageCount, &swapchain.imageCount, swapchain.images), voidFunction);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  // We only create a texture out of the first d3d11 texture
  // Before each rendered frame the native resource is replaced in the ezGALTexture.
  for (ezUInt32 i = 0; i < 1; i++)
  {
    ID3D11Texture2D* pTex = nullptr;
    if (type == SwapchainType::Color)
    {
      pTex = m_colorSwapChainImagesD3D11[i].texture;
    }
    else
    {
      pTex = m_depthSwapChainImagesD3D11[i].texture;
    }

    D3D11_TEXTURE2D_DESC backBufferDesc;
    pTex->GetDesc(&backBufferDesc);

    ezGALTextureCreationDescription textureDesc;
    textureDesc.SetAsRenderTarget(backBufferDesc.Width, backBufferDesc.Height, ConvertTextureFormat(swapchain.format), ezGALMSAASampleCount::Enum(backBufferDesc.SampleDesc.Count));
    textureDesc.m_uiArraySize = backBufferDesc.ArraySize;
    textureDesc.m_pExisitingNativeObject = pTex;
    // Need to add a ref as the ez texture will always remove one on destruction.
    pTex->AddRef();
    if (type == SwapchainType::Color)
    {
      m_hColorRT = pDevice->CreateTexture(textureDesc);
    }
    else
    {
      m_hDepthRT = pDevice->CreateTexture(textureDesc);
    }
  }
  return XR_SUCCESS;
}

XrResult ezOpenXR::InitSwapChain(ezGALMSAASampleCount::Enum msaaCount)
{
  // Read graphics properties for preferred swapchain length and logging.
  XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
  XR_SUCCEED_OR_CLEANUP_LOG(xrGetSystemProperties(m_instance, m_systemId, &systemProperties), DeinitSwapChain);
  m_Info.m_sDeviceName = systemProperties.systemName;

  ezUInt32 viewCount = 0;
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateViewConfigurationViews(m_instance, m_systemId, m_primaryViewConfigurationType, 0, &viewCount, nullptr), DeinitSwapChain);
  if (viewCount != 2)
  {
    ezLog::Error("No stereo view configuration present, can't create swap chain");
    DeinitSwapChain();
    return XR_ERROR_INITIALIZATION_FAILED;
  }
  ezHybridArray<XrViewConfigurationView, 2> views;
  views.SetCount(viewCount, {XR_TYPE_VIEW_CONFIGURATION_VIEW});
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateViewConfigurationViews(m_instance, m_systemId, m_primaryViewConfigurationType, viewCount, &viewCount, views.GetData()), DeinitSwapChain);

  // Create the swapchain and get the images.
  // Select a swapchain format.
  m_primaryConfigView = views[0];
  XR_SUCCEED_OR_CLEANUP_LOG(SelectSwapchainFormat(m_colorSwapchain.format, m_depthSwapchain.format), DeinitSwapChain);

  // Create the swapchain.
  XrSwapchainCreateInfo swapchainCreateInfo{XR_TYPE_SWAPCHAIN_CREATE_INFO};
  swapchainCreateInfo.arraySize = 2;
  swapchainCreateInfo.format = m_colorSwapchain.format;
  swapchainCreateInfo.width = m_primaryConfigView.recommendedImageRectWidth;
  swapchainCreateInfo.height = m_primaryConfigView.recommendedImageRectHeight;
  swapchainCreateInfo.mipCount = 1;
  swapchainCreateInfo.faceCount = 1;
  swapchainCreateInfo.sampleCount = (int)msaaCount;
  swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT;

  m_Info.m_vEyeRenderTargetSize = {swapchainCreateInfo.width, swapchainCreateInfo.height};

  auto CreateSwapChain = [this](const XrSwapchainCreateInfo& swapchainCreateInfo, Swapchain& swapchain, SwapchainType type) -> XrResult {
    XR_SUCCEED_OR_CLEANUP_LOG(xrCreateSwapchain(m_session, &swapchainCreateInfo, &swapchain.handle), voidFunction);
    XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateSwapchainImages(swapchain.handle, 0, &swapchain.imageCount, nullptr), voidFunction);
    CreateSwapchainImages(swapchain, type);

    return XrResult::XR_SUCCESS;
  };
  XR_SUCCEED_OR_CLEANUP_LOG(CreateSwapChain(swapchainCreateInfo, m_colorSwapchain, SwapchainType::Color), DeinitSwapChain);

  if (m_extensions.m_bDepthComposition)
  {
    swapchainCreateInfo.format = m_depthSwapchain.format;
    swapchainCreateInfo.usageFlags = XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    XR_SUCCEED_OR_CLEANUP_LOG(CreateSwapChain(swapchainCreateInfo, m_depthSwapchain, SwapchainType::Depth), DeinitSwapChain);
  }
  else
  {
    // Create depth buffer in case the API does not support it
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    ezGALTextureCreationDescription tcd;
    tcd.SetAsRenderTarget(m_Info.m_vEyeRenderTargetSize.width, m_Info.m_vEyeRenderTargetSize.height, ezGALResourceFormat::DFloat, msaaCount);
    tcd.m_uiArraySize = 2;
    m_hDepthRT = pDevice->CreateTexture(tcd);
  }

  return XrResult::XR_SUCCESS;
}

void ezOpenXR::DeinitSwapChain()
{
  auto DeleteSwapchain = [](Swapchain& swapchain) {
    if (swapchain.handle != XR_NULL_HANDLE)
    {
      xrDestroySwapchain(swapchain.handle);
      swapchain.handle = 0;
    }
    swapchain.format = 0;
    swapchain.imageCount = 0;
    swapchain.images = nullptr;
    swapchain.imageIndex = 0;
  };
  m_primaryConfigView = {XR_TYPE_VIEW_CONFIGURATION_VIEW};
  DeleteSwapchain(m_colorSwapchain);
  DeleteSwapchain(m_depthSwapchain);

  m_colorSwapChainImagesD3D11.Clear();
  m_depthSwapChainImagesD3D11.Clear();
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  if (!m_hColorRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hColorRT);
    m_hColorRT.Invalidate();
  }
  if (!m_hDepthRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hDepthRT);
    m_hDepthRT.Invalidate();
  }
}

void ezOpenXR::BeforeUpdatePlugins()
{
  EZ_PROFILE_SCOPE("BeforeUpdatePlugins");
  // Make sure the main camera component is set to stereo mode.
  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView))
  {
    if (ezWorld* pWorld = pView->GetWorld())
    {
      EZ_LOCK(pWorld->GetWriteMarker());
      ezCameraComponent* pCameraComponent = pWorld->GetComponentManager<ezCameraComponentManager>()->GetCameraByUsageHint(ezCameraUsageHint::MainView);
      EZ_ASSERT_DEV(pCameraComponent != nullptr, "The world must have a main camera component.");
      pCameraComponent->SetCameraMode(ezCameraMode::Stereo);
    }
  }

  XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER, nullptr};

  while (xrPollEvent(m_instance, &event) == XR_SUCCESS)
  {
#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
    m_remoting->HandleEvent(event);
#endif
    switch (event.type)
    {
      case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
      {
        m_Input->UpdateCurrentInteractionProfile();
      }
      break;
      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
      {
        const XrEventDataSessionStateChanged& session_state_changed_event = *reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
        m_sessionState = session_state_changed_event.state;
        switch (m_sessionState)
        {
          case XR_SESSION_STATE_READY:
          {
            XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
            sessionBeginInfo.primaryViewConfigurationType = m_primaryViewConfigurationType;
            if (xrBeginSession(m_session, &sessionBeginInfo) == XR_SUCCESS)
            {
              m_sessionRunning = true;
            }
            break;
          }
          case XR_SESSION_STATE_STOPPING:
          {
            m_sessionRunning = false;
            if (xrEndSession(m_session) != XR_SUCCESS)
            {
              // TODO log
            }
            break;
          }
          case XR_SESSION_STATE_EXITING:
          {
            // Do not attempt to restart because user closed this session.
            m_exitRenderLoop = true;
            m_requestRestart = false;
            break;
          }
          case XR_SESSION_STATE_LOSS_PENDING:
          {
            // Poll for a new systemId
            m_exitRenderLoop = true;
            m_requestRestart = true;
            break;
          }
        }
      }
      break;
      case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
      {
        const XrEventDataInstanceLossPending& instance_loss_pending_event = *reinterpret_cast<XrEventDataInstanceLossPending*>(&event);
        m_exitRenderLoop = true;
        m_requestRestart = false;
      }
      break;
    }
    event = {XR_TYPE_EVENT_DATA_BUFFER, nullptr};
  }

  if (m_exitRenderLoop)
  {
    DeinitSession();
    DeinitSystem();
    if (m_requestRestart)
    {
      // Try to re-init session
      XrResult res = InitSystem();
      if (res != XR_SUCCESS)
      {
        return;
      }
      res = InitSession();
      if (res != XR_SUCCESS)
      {
        DeinitSystem();
        return;
      }
    }
  }
  //#TODO exit render loop and restart logic not fully implemented.
}

void ezOpenXR::UpdatePoses()
{
  EZ_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  EZ_PROFILE_SCOPE("UpdatePoses");
  XrViewState viewState{XR_TYPE_VIEW_STATE};
  ezUInt32 viewCapacityInput = 2;
  ezUInt32 viewCountOutput;

  XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
  viewLocateInfo.viewConfigurationType = m_primaryViewConfigurationType;
  viewLocateInfo.displayTime = m_frameState.predictedDisplayTime;
  viewLocateInfo.space = GetBaseSpace();
  m_views[0].type = XR_TYPE_VIEW;
  m_views[1].type = XR_TYPE_VIEW;
  XrFovf previousFov[2];
  previousFov[0] = m_views[0].fov;
  previousFov[1] = m_views[1].fov;

  XrResult res = xrLocateViews(m_session, &viewLocateInfo, &viewState, viewCapacityInput, &viewCountOutput, m_views);
  m_projectionChanged = ezMemoryUtils::Compare(&previousFov[0], &m_views[0].fov, 1) != 0 || ezMemoryUtils::Compare(&previousFov[1], &m_views[1].fov, 1) != 0;
  if (res != XR_SUCCESS)
  {
    m_Input->m_DeviceState[0].m_bGripPoseIsValid = false;
    m_Input->m_DeviceState[0].m_bAimPoseIsValid = false;
    return;
  }

  UpdateCamera();
  m_Input->UpdateActions();
  if (m_HandTracking)
  {
    m_HandTracking->UpdateJointTransforms();
  }
}

void ezOpenXR::UpdateCamera()
{
  if (!m_pCameraToSynchronize)
  {
    m_Input->m_DeviceState[0].m_bGripPoseIsValid = false;
    m_Input->m_DeviceState[0].m_bDeviceIsConnected = false;
    return;
  }
  // Update camera projection
  if (m_uiSettingsModificationCounter != m_pCameraToSynchronize->GetSettingsModificationCounter() || m_projectionChanged)
  {
    m_projectionChanged = false;
    const float fAspectRatio = (float)m_Info.m_vEyeRenderTargetSize.width / (float)m_Info.m_vEyeRenderTargetSize.height;
    auto CreateProjection = [](const XrView& view, ezCamera* cam) {
      return ezGraphicsUtils::CreatePerspectiveProjectionMatrix(ezMath::Tan(ezAngle::Radian(view.fov.angleLeft)) * cam->GetNearPlane(), ezMath::Tan(ezAngle::Radian(view.fov.angleRight)) * cam->GetNearPlane(), ezMath::Tan(ezAngle::Radian(view.fov.angleDown)) * cam->GetNearPlane(),
        ezMath::Tan(ezAngle::Radian(view.fov.angleUp)) * cam->GetNearPlane(), cam->GetNearPlane(), cam->GetFarPlane());
    };

    // Update projection with newest near/ far values. If not sync camera is set, just use the last value from XR
    // camera.
    const ezMat4 projLeft = CreateProjection(m_views[0], m_pCameraToSynchronize);
    const ezMat4 projRight = CreateProjection(m_views[1], m_pCameraToSynchronize);
    m_pCameraToSynchronize->SetStereoProjection(projLeft, projRight, fAspectRatio);
    m_uiSettingsModificationCounter = m_pCameraToSynchronize->GetSettingsModificationCounter();
  }

  // Update camera view
  {
    ezTransform add;
    add.SetIdentity();
    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(m_hView, pView))
    {
      if (const ezWorld* pWorld = pView->GetWorld())
      {
        EZ_LOCK(pWorld->GetReadMarker());
        if (const ezStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<ezStageSpaceComponentManager>())
        {
          if (const ezStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
          {
            ezEnum<ezXRStageSpace> stageSpace = pStage->GetStageSpace();
            if (m_StageSpace != stageSpace)
              SetStageSpace(pStage->GetStageSpace());
            add = pStage->GetOwner()->GetGlobalTransform();
          }
        }
      }
    }

    {
      // Update device state (average of both eyes).
      ezQuat rot;
      rot.SetIdentity();
      rot.SetSlerp(ConvertOrientation(m_views[0].pose.orientation), ConvertOrientation(m_views[1].pose.orientation), 0.5f);
      const ezVec3 pos = ezMath::Lerp(ConvertPosition(m_views[0].pose.position), ConvertPosition(m_views[1].pose.position), 0.5f);

      m_Input->m_DeviceState[0].m_vGripPosition = pos;
      m_Input->m_DeviceState[0].m_qGripRotation = rot;
      m_Input->m_DeviceState[0].m_vAimPosition = pos;
      m_Input->m_DeviceState[0].m_qAimRotation = rot;
      m_Input->m_DeviceState[0].m_Type = ezXRDeviceType::HMD;
      m_Input->m_DeviceState[0].m_bGripPoseIsValid = true;
      m_Input->m_DeviceState[0].m_bAimPoseIsValid = true;
      m_Input->m_DeviceState[0].m_bDeviceIsConnected = true;
    }

    // Set view matrix
    {
      const ezMat4 mStageTransform = add.GetAsMat4();
      const ezMat4 poseLeft = mStageTransform * ConvertPoseToMatrix(m_views[0].pose);
      const ezMat4 poseRight = mStageTransform * ConvertPoseToMatrix(m_views[1].pose);

      // EZ Forward is +X, need to add this to align the forward projection
      const ezMat4 viewMatrix = ezGraphicsUtils::CreateLookAtViewMatrix(ezVec3::ZeroVector(), ezVec3(1, 0, 0), ezVec3(0, 0, 1));
      const ezMat4 mViewTransformLeft = viewMatrix * poseLeft.GetInverse();
      const ezMat4 mViewTransformRight = viewMatrix * poseRight.GetInverse();

      m_pCameraToSynchronize->SetViewMatrix(mViewTransformLeft, ezCameraEye::Left);
      m_pCameraToSynchronize->SetViewMatrix(mViewTransformRight, ezCameraEye::Right);
    }
  }
}

void ezOpenXR::BeforeBeginFrame()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  winrt::Windows::UI::Core::CoreWindow window = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
  window.Dispatcher().ProcessEvents(winrt::Windows::UI::Core::CoreProcessEventsOption::ProcessAllIfPresent);
#endif

  if (m_hView.IsInvalidated() || !m_sessionRunning)
    return;

  EZ_PROFILE_SCOPE("BeforeBeginFrame");
  {
    EZ_PROFILE_SCOPE("xrWaitFrame");
    m_frameWaitInfo = XrFrameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
    m_frameState = XrFrameState{XR_TYPE_FRAME_STATE};
    XrResult result = xrWaitFrame(m_session, &m_frameWaitInfo, &m_frameState);
    if (result != XR_SUCCESS)
    {
      m_renderInProgress = false;
      return;
    }
  }
  {
    EZ_PROFILE_SCOPE("xrBeginFrame");
    m_frameBeginInfo = XrFrameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    XrResult result = xrBeginFrame(m_session, &m_frameBeginInfo);
    if (result != XR_SUCCESS)
    {
      m_renderInProgress = false;
      return;
    }
  }

  auto AquireAndWait = [](Swapchain& swapchain) {
    XrSwapchainImageAcquireInfo acquireInfo{XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    XR_SUCCEED_OR_RETURN_LOG(xrAcquireSwapchainImage(swapchain.handle, &acquireInfo, &swapchain.imageIndex));

    XrSwapchainImageWaitInfo waitInfo{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO};
    waitInfo.timeout = XR_INFINITE_DURATION;
    XR_SUCCEED_OR_RETURN_LOG(xrWaitSwapchainImage(swapchain.handle, &waitInfo));
    return XR_SUCCESS;
  };

  {
    EZ_PROFILE_SCOPE("AquireAndWait");
    AquireAndWait(m_colorSwapchain);
    if (m_extensions.m_bDepthComposition)
      AquireAndWait(m_depthSwapchain);
  }
  UpdatePoses();

  // This will update the extracted view from last frame with the new data we got
  // this frame just before starting to render.
  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView))
  {
    // Update render target setup with current OpenXR swapchain texture.
    ID3D11Texture2D* pTex = m_colorSwapChainImagesD3D11[m_colorSwapchain.imageIndex].texture;
    pTex->AddRef();
    ezGALDevice::GetDefaultDevice()->ReplaceExisitingNativeObject(m_hColorRT, pTex).IgnoreResult();

    if (m_extensions.m_bDepthComposition)
    {
      ID3D11Texture2D* pTex = m_depthSwapChainImagesD3D11[m_depthSwapchain.imageIndex].texture;
      pTex->AddRef();
      ezGALDevice::GetDefaultDevice()->ReplaceExisitingNativeObject(m_hDepthRT, pTex).IgnoreResult();
    }

    pView->UpdateViewData(ezRenderWorld::GetDataIndexForRendering());
  }
  m_renderInProgress = true;
}

ezGALTextureHandle ezOpenXR::Present()
{
  if (!m_renderInProgress)
    return {};

  EZ_PROFILE_SCOPE("BeforePresent");
  for (uint32_t i = 0; i < 2; i++)
  {
    m_projectionLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
    m_projectionLayerViews[i].pose = m_views[i].pose;
    m_projectionLayerViews[i].fov = m_views[i].fov;
    m_projectionLayerViews[i].subImage.swapchain = m_colorSwapchain.handle;
    m_projectionLayerViews[i].subImage.imageRect.offset = {0, 0};
    m_projectionLayerViews[i].subImage.imageRect.extent = {(ezInt32)m_Info.m_vEyeRenderTargetSize.width, (ezInt32)m_Info.m_vEyeRenderTargetSize.height};
    m_projectionLayerViews[i].subImage.imageArrayIndex = i;

    if (m_extensions.m_bDepthComposition && m_pCameraToSynchronize)
    {
      m_depthLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR};
      m_depthLayerViews[i].minDepth = 0;
      m_depthLayerViews[i].maxDepth = 1;
      m_depthLayerViews[i].nearZ = m_pCameraToSynchronize->GetNearPlane();
      m_depthLayerViews[i].farZ = m_pCameraToSynchronize->GetFarPlane();
      m_depthLayerViews[i].subImage.swapchain = m_depthSwapchain.handle;
      m_depthLayerViews[i].subImage.imageRect.offset = {0, 0};
      m_depthLayerViews[i].subImage.imageRect.extent = {(ezInt32)m_Info.m_vEyeRenderTargetSize.width, (ezInt32)m_Info.m_vEyeRenderTargetSize.height};
      m_depthLayerViews[i].subImage.imageArrayIndex = i;

      m_projectionLayerViews[i].next = &m_depthLayerViews[i];
    }
  }

  {
    EZ_PROFILE_SCOPE("xrReleaseSwapchainImage");
    XrSwapchainImageReleaseInfo releaseInfo{XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    XR_LOG_ERROR(xrReleaseSwapchainImage(m_colorSwapchain.handle, &releaseInfo));
    if (m_extensions.m_bDepthComposition)
    {
      XR_LOG_ERROR(xrReleaseSwapchainImage(m_depthSwapchain.handle, &releaseInfo));
    }
  }
  m_layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
  m_layer.space = GetBaseSpace();
  m_layer.viewCount = 2;
  m_layer.views = m_projectionLayerViews;

  ezHybridArray<XrCompositionLayerBaseHeader*, 1> layers;
  layers.PushBack(reinterpret_cast<XrCompositionLayerBaseHeader*>(&m_layer));

  // Submit the composition layers for the predicted display time.
  XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
  frameEndInfo.displayTime = m_frameState.predictedDisplayTime;
  frameEndInfo.environmentBlendMode = m_blendMode;
  frameEndInfo.layerCount = layers.GetCapacity();
  frameEndInfo.layers = layers.GetData();

  EZ_PROFILE_SCOPE("xrEndFrame");
  XR_LOG_ERROR(xrEndFrame(m_session, &frameEndInfo));

  return m_hColorRT;
}

void ezOpenXR::GALDeviceEventHandler(const ezGALDeviceEvent& e)
{
  if (e.m_Type == ezGALDeviceEvent::Type::BeforeBeginFrame)
  {
    BeforeBeginFrame();
  }
}

void ezOpenXR::GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e)
{
  EZ_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  if (e.m_Type == ezGameApplicationExecutionEvent::Type::BeforeUpdatePlugins)
  {
    BeforeUpdatePlugins();
  }
}

void ezOpenXR::SetStageSpace(ezXRStageSpace::Enum space)
{
  m_StageSpace = space;
}

void ezOpenXR::SetHMDCamera(ezCamera* pCamera)
{
  EZ_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  if (m_pCameraToSynchronize == pCamera)
    return;

  m_pCameraToSynchronize = pCamera;
  if (m_pCameraToSynchronize)
  {
    m_uiSettingsModificationCounter = m_pCameraToSynchronize->GetSettingsModificationCounter() + 1;
    m_pCameraToSynchronize->SetCameraMode(ezCameraMode::Stereo, m_pCameraToSynchronize->GetFovOrDim(), m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
  }
}

XrPosef ezOpenXR::ConvertTransform(const ezTransform& tr)
{
  XrPosef pose;
  pose.orientation = ConvertOrientation(tr.m_qRotation);
  pose.position = ConvertPosition(tr.m_vPosition);
  return pose;
}

XrQuaternionf ezOpenXR::ConvertOrientation(const ezQuat& q)
{
  return {q.v.y, q.v.z, -q.v.x, -q.w};
}

XrVector3f ezOpenXR::ConvertPosition(const ezVec3& pos)
{
  return {pos.y, pos.z, -pos.x};
}

ezQuat ezOpenXR::ConvertOrientation(const XrQuaternionf& q)
{
  return {-q.z, q.x, q.y, -q.w};
}

ezVec3 ezOpenXR::ConvertPosition(const XrVector3f& pos)
{
  return {-pos.z, pos.x, pos.y};
}

ezMat4 ezOpenXR::ConvertPoseToMatrix(const XrPosef& pose)
{
  ezMat4 m;
  ezMat3 rot = ConvertOrientation(pose.orientation).GetAsMat3();
  ezVec3 pos = ConvertPosition(pose.position);
  m.SetTransformationMatrix(rot, pos);
  return m;
}

ezGALResourceFormat::Enum ezOpenXR::ConvertTextureFormat(int64_t format)
{
  switch (format)
  {
    case DXGI_FORMAT_D32_FLOAT:
      return ezGALResourceFormat::DFloat;
    case DXGI_FORMAT_D16_UNORM:
      return ezGALResourceFormat::D16;
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
      return ezGALResourceFormat::D24S8;
    default:
      ezImageFormat::Enum imageFormat = ezImageFormatMappings::FromDxgiFormat(static_cast<ezUInt32>(format));
      ezGALResourceFormat::Enum galFormat = ezTextureUtils::ImageFormatToGalFormat(imageFormat, false);
      return galFormat;
  }
}

EZ_STATICLINK_FILE(OpenXRPlugin, OpenXRPlugin_OpenXRSingleton);
