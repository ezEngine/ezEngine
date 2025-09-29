#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Core/System/WindowManager.h>
#include <Core/World/World.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <GameEngine/XR/XRWindow.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRHandTracking.h>
#include <OpenXRPlugin/OpenXRInputDevice.h>
#include <OpenXRPlugin/OpenXRRemoting.h>
#include <OpenXRPlugin/OpenXRSingleton.h>
#include <OpenXRPlugin/OpenXRSpatialAnchors.h>
#include <OpenXRPlugin/OpenXRSwapChain.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureUtils.h>
#include <Texture/Image/Formats/ImageFormatMappings.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <RendererDX11/Device/DeviceDX11.h>
#endif

#include <vector>

static_assert(ezGALMSAASampleCount::None == 1);
static_assert(ezGALMSAASampleCount::TwoSamples == 2);
static_assert(ezGALMSAASampleCount::FourSamples == 4);
static_assert(ezGALMSAASampleCount::EightSamples == 8);

EZ_IMPLEMENT_SINGLETON(ezOpenXR);

static ezOpenXR g_OpenXRSingleton;

XrBool32 XRAPI_CALL xrDebugCallback(XrDebugUtilsMessageSeverityFlagsEXT messageSeverity, XrDebugUtilsMessageTypeFlagsEXT messageTypes, const XrDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
  switch (messageSeverity)
  {
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
      ezLog::Debug("XR: {}", pCallbackData->message);
      break;
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
      ezLog::Info("XR: {}", pCallbackData->message);
      break;
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
      ezLog::Warning("XR: {}", pCallbackData->message);
      break;
    case XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
      ezLog::Error("XR: {}", pCallbackData->message);
      break;
    default:
      break;
  }
  // Only layers are allowed to return true here.
  return XR_FALSE;
}

ezOpenXR::ezOpenXR()
  : m_SingletonRegistrar(this)
{
#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  m_pRemoting = EZ_DEFAULT_NEW(ezOpenXRRemoting, this);
#endif
}

ezOpenXR::~ezOpenXR() = default;

bool ezOpenXR::GetDepthComposition() const
{
  return m_Extensions.m_bDepthComposition;
}

bool ezOpenXR::IsHmdPresent() const
{
  XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
  systemInfo.formFactor = XrFormFactor::XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  uint64_t systemId = XR_NULL_SYSTEM_ID;
  XrResult res = xrGetSystem(m_pInstance, &systemInfo, &systemId);

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
  auto AddExtIfSupported = [&](const char* extensionName, bool& enableFlag) -> XrResult
  {
    auto it = std::find_if(begin(extensionProperties), end(extensionProperties), [&](const XrExtensionProperties& prop)
      { return ezStringUtils::IsEqual(prop.extensionName, extensionName); });
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
  XR_SUCCEED_OR_RETURN_LOG(AddExtIfSupported(XR_KHR_D3D11_ENABLE_EXTENSION_NAME, m_Extensions.m_bD3D11));

  AddExtIfSupported(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME, m_Extensions.m_bDepthComposition);
  AddExtIfSupported(XR_MSFT_UNBOUNDED_REFERENCE_SPACE_EXTENSION_NAME, m_Extensions.m_bUnboundedReferenceSpace);
  AddExtIfSupported(XR_MSFT_SPATIAL_ANCHOR_EXTENSION_NAME, m_Extensions.m_bSpatialAnchor);
  AddExtIfSupported(XR_EXT_HAND_TRACKING_EXTENSION_NAME, m_Extensions.m_bHandTracking);
  AddExtIfSupported(XR_MSFT_HAND_INTERACTION_EXTENSION_NAME, m_Extensions.m_bHandInteraction);
  AddExtIfSupported(XR_MSFT_HAND_TRACKING_MESH_EXTENSION_NAME, m_Extensions.m_bHandTrackingMesh);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  AddExtIfSupported(XR_EXT_DEBUG_UTILS_EXTENSION_NAME, m_Extensions.m_bDebugUtils);
#endif

#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  AddExtIfSupported(XR_MSFT_HOLOGRAPHIC_REMOTING_EXTENSION_NAME, m_Extensions.m_bRemoting);
#endif

#ifdef BUILDSYSTEM_ENABLE_OPENXR_PREVIEW_SUPPORT
#endif
  return XR_SUCCESS;
}

XrResult ezOpenXR::SelectLayers(ezHybridArray<const char*, 6>& layers)
{
  ezUInt32 layerCount;
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateApiLayerProperties(0, &layerCount, nullptr));
  std::vector<XrApiLayerProperties> layerProperties(layerCount, {XR_TYPE_API_LAYER_PROPERTIES});
  XR_SUCCEED_OR_RETURN_LOG(xrEnumerateApiLayerProperties(layerCount, &layerCount, layerProperties.data()));

  // Add a specific extension to the list of extensions to be enabled, if it is supported.
  auto AddExtIfSupported = [&](const char* layerName, bool& enableFlag) -> XrResult
  {
    auto it = std::find_if(begin(layerProperties), end(layerProperties), [&](const XrApiLayerProperties& prop)
      { return ezStringUtils::IsEqual(prop.layerName, layerName); });
    if (it != end(layerProperties))
    {
      layers.PushBack(layerName);
      enableFlag = true;
      return XR_SUCCESS;
    }
    enableFlag = false;
    return XR_ERROR_EXTENSION_NOT_PRESENT;
  };

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  AddExtIfSupported("XR_APILAYER_LUNARG_core_validation", m_Extensions.m_bValidation);
#endif

  return XR_SUCCESS;
}

#define EZ_GET_INSTANCE_PROC_ADDR(name) (void)xrGetInstanceProcAddr(m_pInstance, #name, reinterpret_cast<PFN_xrVoidFunction*>(&m_Extensions.pfn_##name));

ezResult ezOpenXR::Initialize()
{
  if (m_pInstance != XR_NULL_HANDLE)
    return EZ_SUCCESS;

  // Build out the extensions to enable. Some extensions are required and some are optional.
  ezHybridArray<const char*, 6> enabledExtensions;
  if (SelectExtensions(enabledExtensions) != XR_SUCCESS)
    return EZ_FAILURE;

  ezHybridArray<const char*, 6> enabledLayers;
  if (SelectLayers(enabledLayers) != XR_SUCCESS)
    return EZ_FAILURE;

  // Create the instance with desired extensions.
  XrInstanceCreateInfo createInfo{XR_TYPE_INSTANCE_CREATE_INFO};
  createInfo.enabledExtensionCount = (uint32_t)enabledExtensions.GetCount();
  createInfo.enabledExtensionNames = enabledExtensions.GetData();
  createInfo.enabledApiLayerCount = (uint32_t)enabledLayers.GetCount();
  createInfo.enabledApiLayerNames = enabledLayers.GetData();

  ezStringUtils::Copy(createInfo.applicationInfo.applicationName, EZ_ARRAY_SIZE(createInfo.applicationInfo.applicationName), ezApplication::GetApplicationInstance()->GetApplicationName());
  ezStringUtils::Copy(createInfo.applicationInfo.engineName, EZ_ARRAY_SIZE(createInfo.applicationInfo.engineName), "ezEngine");
  createInfo.applicationInfo.engineVersion = 1;
  createInfo.applicationInfo.apiVersion = XR_CURRENT_API_VERSION;
  createInfo.applicationInfo.applicationVersion = 1;
  XrResult res = xrCreateInstance(&createInfo, &m_pInstance);
  if (res != XR_SUCCESS)
  {
    ezLog::Error("InitSystem xrCreateInstance failed: {}", res);
    Deinitialize();
    return EZ_FAILURE;
  }
  XrInstanceProperties instanceProperties{XR_TYPE_INSTANCE_PROPERTIES};
  res = xrGetInstanceProperties(m_pInstance, &instanceProperties);
  if (res != XR_SUCCESS)
  {
    ezLog::Error("InitSystem xrGetInstanceProperties failed: {}", res);
    Deinitialize();
    return EZ_FAILURE;
  }

  ezStringBuilder sTemp;
  m_Info.m_sDeviceDriver = ezConversionUtils::ToString(instanceProperties.runtimeVersion, sTemp);

  EZ_GET_INSTANCE_PROC_ADDR(xrGetD3D11GraphicsRequirementsKHR);

  if (m_Extensions.m_bSpatialAnchor)
  {
    EZ_GET_INSTANCE_PROC_ADDR(xrCreateSpatialAnchorMSFT);
    EZ_GET_INSTANCE_PROC_ADDR(xrCreateSpatialAnchorSpaceMSFT);
    EZ_GET_INSTANCE_PROC_ADDR(xrDestroySpatialAnchorMSFT);
  }

  if (m_Extensions.m_bHandTracking)
  {
    EZ_GET_INSTANCE_PROC_ADDR(xrCreateHandTrackerEXT);
    EZ_GET_INSTANCE_PROC_ADDR(xrDestroyHandTrackerEXT);
    EZ_GET_INSTANCE_PROC_ADDR(xrLocateHandJointsEXT);
  }

  if (m_Extensions.m_bHandTrackingMesh)
  {
    EZ_GET_INSTANCE_PROC_ADDR(xrCreateHandMeshSpaceMSFT);
    EZ_GET_INSTANCE_PROC_ADDR(xrUpdateHandMeshMSFT);
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (m_Extensions.m_bDebugUtils)
  {
    EZ_GET_INSTANCE_PROC_ADDR(xrCreateDebugUtilsMessengerEXT);
    EZ_GET_INSTANCE_PROC_ADDR(xrDestroyDebugUtilsMessengerEXT);
  }
#endif

#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  if (m_Extensions.m_bRemoting)
  {
    EZ_GET_INSTANCE_PROC_ADDR(xrRemotingSetContextPropertiesMSFT);
    EZ_GET_INSTANCE_PROC_ADDR(xrRemotingConnectMSFT);
    EZ_GET_INSTANCE_PROC_ADDR(xrRemotingDisconnectMSFT);
    EZ_GET_INSTANCE_PROC_ADDR(xrRemotingGetConnectionStateMSFT);
  }
#endif

  m_pInput = EZ_DEFAULT_NEW(ezOpenXRInputDevice, this);

  m_ExecutionEventsId = ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(ezMakeDelegate(&ezOpenXR::GameApplicationEventHandler, this));

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
  m_pRemoting->Disconnect().IgnoreResult();
#endif

  if (m_ExecutionEventsId != 0)
  {
    ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(m_ExecutionEventsId);
  }

  ezGALDevice::GetDefaultDevice()->DestroySwapChain(m_hSwapChain);
  m_hSwapChain.Invalidate();

  DeinitSession();
  DeinitSystem();

  m_pInput = nullptr;

  if (m_pInstance)
  {
    xrDestroyInstance(m_pInstance);
    m_pInstance = XR_NULL_HANDLE;
  }
}

bool ezOpenXR::IsInitialized() const
{
  return m_pInstance != XR_NULL_HANDLE;
}

const ezHMDInfo& ezOpenXR::GetHmdInfo() const
{
  EZ_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");
  return m_Info;
}

ezXRInputDevice& ezOpenXR::GetXRInput() const
{
  return *(m_pInput.Borrow());
}

ezRegisteredWndHandle ezOpenXR::CreateXRWindow(ezView* pView, ezGALMSAASampleCount::Enum msaaCount, ezUniquePtr<ezWindowBase> pCompanionWindow, ezUniquePtr<ezWindowOutputTargetGAL> pCompanionWindowOutput)
{
  EZ_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  XrResult res = InitSession();
  if (res != XrResult::XR_SUCCESS)
  {
    ezLog::Error("InitSession failed: {}", res);
    return {};
  }

  ezGALXRSwapChain::SetFactoryMethod([this, msaaCount](ezXRInterface* pXrInterface) -> ezGALSwapChainHandle
    { return ezGALDevice::GetDefaultDevice()->CreateSwapChain([this, pXrInterface, msaaCount](ezAllocator* pAllocator) -> ezGALSwapChain*
        { return EZ_NEW(pAllocator, ezGALOpenXRSwapChain, this, msaaCount); }); });
  EZ_SCOPE_EXIT(ezGALXRSwapChain::SetFactoryMethod({}););

  m_hSwapChain = ezGALXRSwapChain::Create(this);
  if (m_hSwapChain.IsInvalidated())
  {
    DeinitSession();
    ezLog::Error("InitSwapChain failed: {}", res);
    return {};
  }

  const ezGALOpenXRSwapChain* pSwapChain = static_cast<const ezGALOpenXRSwapChain*>(ezGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain));
  m_Info.m_vEyeRenderTargetSize = pSwapChain->GetRenderTargetSize();

  {
    EZ_ASSERT_DEV(pView->GetCamera() != nullptr, "The provided view requires a camera to be set.");
    SetHMDCamera(pView->GetCamera());
  }

  auto pWinMan = ezWindowManager::GetSingleton();

  EZ_ASSERT_DEV((pCompanionWindow != nullptr) == (pCompanionWindowOutput != nullptr), "Both companionWindow and companionWindowOutput must either be null or valid.");
  EZ_ASSERT_DEV(pCompanionWindow == nullptr || SupportsCompanionView(), "If a companionWindow is set, SupportsCompanionView() must be true.");

  ezUniquePtr<ezWindowXR> pWindowXR = EZ_DEFAULT_NEW(ezWindowXR, this, std::move(pCompanionWindow));
  ezUniquePtr<ezWindowOutputTargetXR> pOutputTargetXR = EZ_DEFAULT_NEW(ezWindowOutputTargetXR, this, std::move(pCompanionWindowOutput));

  m_pCompanion = static_cast<ezWindowOutputTargetXR*>(pOutputTargetXR.Borrow());

  ezRegisteredWndHandle windowId = pWinMan->Register("OpenXR", this, std::move(pWindowXR));
  pWinMan->SetOutputTarget(windowId, std::move(pOutputTargetXR));
  pWinMan->SetDestroyCallback(windowId, [this](ezRegisteredWndHandle)
    { this->OnActorDestroyed(); });

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  m_hView = pView->GetHandle();

  pView->SetSwapChain(m_hSwapChain);

  pView->SetViewport(ezRectFloat((float)m_Info.m_vEyeRenderTargetSize.width, (float)m_Info.m_vEyeRenderTargetSize.height));

  return windowId;
}

void ezOpenXR::OnActorDestroyed()
{
  if (m_hView.IsInvalidated())
    return;

  m_pCompanion = nullptr;
  SetHMDCamera(nullptr);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezRenderWorld::RemoveMainView(m_hView);
  m_hView.Invalidate();

  // #TODO_XR DestroySwapChain will only queue the destruction. We either need to flush it somehow or postpone DeinitSession.
  ezGALDevice::GetDefaultDevice()->DestroySwapChain(m_hSwapChain);
  m_hSwapChain.Invalidate();

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
  return m_StageSpace == ezXRStageSpace::Standing ? m_pSceneSpace : m_pLocalSpace;
}

XrResult ezOpenXR::InitSystem()
{
  EZ_ASSERT_DEV(m_SystemId == XR_NULL_SYSTEM_ID, "OpenXR actor already exists.");
  XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
  systemInfo.formFactor = XrFormFactor::XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  XR_SUCCEED_OR_CLEANUP_LOG(xrGetSystem(m_pInstance, &systemInfo, &m_SystemId), DeinitSystem);

  XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
  XR_SUCCEED_OR_CLEANUP_LOG(xrGetSystemProperties(m_pInstance, m_SystemId, &systemProperties), DeinitSystem);
  m_Info.m_sDeviceName = systemProperties.systemName;

  return XrResult::XR_SUCCESS;
}

void ezOpenXR::DeinitSystem()
{
  m_SystemId = XR_NULL_SYSTEM_ID;
}

XrResult ezOpenXR::InitSession()
{
  EZ_ASSERT_DEV(m_pSession == XR_NULL_HANDLE, "");

  ezUInt32 count;
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateEnvironmentBlendModes(m_pInstance, m_SystemId, m_PrimaryViewConfigurationType, 0, &count, nullptr), DeinitSystem);

  ezHybridArray<XrEnvironmentBlendMode, 4> environmentBlendModes;
  environmentBlendModes.SetCount(count);
  XR_SUCCEED_OR_CLEANUP_LOG(xrEnumerateEnvironmentBlendModes(m_pInstance, m_SystemId, m_PrimaryViewConfigurationType, count, &count, environmentBlendModes.GetData()), DeinitSession);

  // Select preferred blend mode.
  m_BlendMode = environmentBlendModes[0];

  XR_SUCCEED_OR_CLEANUP_LOG(InitGraphicsPlugin(), DeinitSession);
  XR_SUCCEED_OR_CLEANUP_LOG(InitDebugMessenger(), DeinitSession);

  XrSessionCreateInfo sessionCreateInfo{XR_TYPE_SESSION_CREATE_INFO};
  sessionCreateInfo.systemId = m_SystemId;
  sessionCreateInfo.next = &m_XrGraphicsBindingD3D11;

  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateSession(m_pInstance, &sessionCreateInfo, &m_pSession), DeinitSession);

  XrReferenceSpaceCreateInfo spaceCreateInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
  spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
  spaceCreateInfo.poseInReferenceSpace = ConvertTransform(ezTransform::MakeIdentity());
  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateReferenceSpace(m_pSession, &spaceCreateInfo, &m_pSceneSpace), DeinitSession);

  spaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateReferenceSpace(m_pSession, &spaceCreateInfo, &m_pLocalSpace), DeinitSession);

  XR_SUCCEED_OR_CLEANUP_LOG(m_pInput->CreateActions(m_pSession, m_pSceneSpace), DeinitSession);
  XR_SUCCEED_OR_CLEANUP_LOG(m_pInput->AttachSessionActionSets(m_pSession), DeinitSession);

  m_GALdeviceEventsId = ezGALDevice::s_Events.AddEventHandler(ezMakeDelegate(&ezOpenXR::GALDeviceEventHandler, this));

  SetStageSpace(ezXRStageSpace::Standing);
  if (m_Extensions.m_bSpatialAnchor)
  {
    m_pAnchors = EZ_DEFAULT_NEW(ezOpenXRSpatialAnchors, this);
  }
  if (m_Extensions.m_bHandTracking && ezOpenXRHandTracking::IsHandTrackingSupported(this))
  {
    m_pHandTracking = EZ_DEFAULT_NEW(ezOpenXRHandTracking, this);
  }
  return XrResult::XR_SUCCESS;
}

void ezOpenXR::DeinitSession()
{
  m_pCompanion = nullptr;
  m_bSessionRunning = false;
  m_bExitRenderLoop = false;
  m_bRequestRestart = false;
  m_bRenderInProgress = false;
  m_SessionState = XR_SESSION_STATE_UNKNOWN;

  m_pHandTracking = nullptr;
  m_pAnchors = nullptr;
  if (m_GALdeviceEventsId != 0)
  {
    ezGALDevice::s_Events.RemoveEventHandler(m_GALdeviceEventsId);
  }

  if (m_pSceneSpace)
  {
    xrDestroySpace(m_pSceneSpace);
    m_pSceneSpace = XR_NULL_HANDLE;
  }

  if (m_pLocalSpace)
  {
    xrDestroySpace(m_pLocalSpace);
    m_pLocalSpace = XR_NULL_HANDLE;
  }

  m_pInput->DestroyActions();

  if (m_pSession)
  {
    // #TODO_XR flush command queue
    xrDestroySession(m_pSession);
    m_pSession = XR_NULL_HANDLE;
  }

  DeinitGraphicsPlugin();
  DeinitInitDebugMessenger();
}

XrResult ezOpenXR::InitGraphicsPlugin()
{
  EZ_ASSERT_DEV(m_XrGraphicsBindingD3D11.device == nullptr, "");
  // Hard-coded to d3d
  XrGraphicsRequirementsD3D11KHR graphicsRequirements{XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR};
  XR_SUCCEED_OR_CLEANUP_LOG(m_Extensions.pfn_xrGetD3D11GraphicsRequirementsKHR(m_pInstance, m_SystemId, &graphicsRequirements), DeinitGraphicsPlugin);
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
  ezGALDeviceDX11* pD3dDevice = static_cast<ezGALDeviceDX11*>(pDevice);

  m_XrGraphicsBindingD3D11.device = pD3dDevice->GetDXDevice();

  return XrResult::XR_SUCCESS;
}

void ezOpenXR::DeinitGraphicsPlugin()
{
  m_XrGraphicsBindingD3D11.device = nullptr;
}

XrResult ezOpenXR::InitDebugMessenger()
{
  XrDebugUtilsMessengerCreateInfoEXT create_info{XR_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
  create_info.messageSeverities = XR_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  XR_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                  XR_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  XR_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info.messageTypes = XR_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             XR_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             XR_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info.userCallback = xrDebugCallback;

  XR_SUCCEED_OR_CLEANUP_LOG(m_Extensions.pfn_xrCreateDebugUtilsMessengerEXT(m_pInstance, &create_info, &m_pDebugMessenger), DeinitInitDebugMessenger);

  return XrResult::XR_SUCCESS;
}

void ezOpenXR::DeinitInitDebugMessenger()
{
  if (m_pDebugMessenger != XR_NULL_HANDLE)
  {
    XR_LOG_ERROR(m_Extensions.pfn_xrDestroyDebugUtilsMessengerEXT(m_pDebugMessenger));
    m_pDebugMessenger = XR_NULL_HANDLE;
  }
}

void ezOpenXR::BeforeUpdatePlugins()
{
  EZ_PROFILE_SCOPE("BeforeUpdatePlugins");
  // Make sure the main camera component is set to stereo mode.
  if (ezWorld* pWorld = GetWorld())
  {
    EZ_LOCK(pWorld->GetWriteMarker());
    auto* pCCM = pWorld->GetComponentManager<ezCameraComponentManager>();
    if (pCCM)
    {
      if (ezCameraComponent* pCameraComponent = pCCM->GetCameraByUsageHint(ezCameraUsageHint::MainView))
      {
        pCameraComponent->SetCameraMode(ezCameraMode::Stereo);
      }
    }
  }

  XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER, nullptr};

  while (xrPollEvent(m_pInstance, &event) == XR_SUCCESS)
  {
#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
    m_pRemoting->HandleEvent(event);
#endif
    switch (event.type)
    {
      case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
      {
        m_pInput->UpdateCurrentInteractionProfile();
      }
      break;
      case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
      {
        const XrEventDataSessionStateChanged& session_state_changed_event = *reinterpret_cast<XrEventDataSessionStateChanged*>(&event);
        m_SessionState = session_state_changed_event.state;
        switch (m_SessionState)
        {
          case XR_SESSION_STATE_READY:
          {
            XrSessionBeginInfo sessionBeginInfo{XR_TYPE_SESSION_BEGIN_INFO};
            sessionBeginInfo.primaryViewConfigurationType = m_PrimaryViewConfigurationType;
            if (xrBeginSession(m_pSession, &sessionBeginInfo) == XR_SUCCESS)
            {
              m_bSessionRunning = true;
            }
            break;
          }
          case XR_SESSION_STATE_STOPPING:
          {
            m_bSessionRunning = false;
            if (xrEndSession(m_pSession) != XR_SUCCESS)
            {
              // TODO log
            }
            break;
          }
          case XR_SESSION_STATE_EXITING:
          {
            // Do not attempt to restart because user closed this session.
            m_bExitRenderLoop = true;
            m_bRequestRestart = false;
            break;
          }
          case XR_SESSION_STATE_LOSS_PENDING:
          {
            // Poll for a new systemId
            m_bExitRenderLoop = true;
            m_bRequestRestart = true;
            break;
          }
          default:
            break;
        }
      }
      break;
      case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
      {
        const XrEventDataInstanceLossPending& instance_loss_pending_event = *reinterpret_cast<XrEventDataInstanceLossPending*>(&event);
        m_bExitRenderLoop = true;
        m_bRequestRestart = false;
      }
      break;
      default:
        break;
    }
    event = {XR_TYPE_EVENT_DATA_BUFFER, nullptr};
  }

  if (m_bExitRenderLoop)
  {
    DeinitSession();
    DeinitSystem();
    if (m_bRequestRestart)
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
  // #TODO exit render loop and restart logic not fully implemented.
}

void ezOpenXR::UpdatePoses()
{
  EZ_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");

  EZ_PROFILE_SCOPE("UpdatePoses");
  m_ViewState = XrViewState{XR_TYPE_VIEW_STATE};
  ezUInt32 viewCapacityInput = 2;
  ezUInt32 viewCountOutput;

  XrViewLocateInfo viewLocateInfo{XR_TYPE_VIEW_LOCATE_INFO};
  viewLocateInfo.viewConfigurationType = m_PrimaryViewConfigurationType;
  viewLocateInfo.displayTime = m_FrameState.predictedDisplayTime;
  viewLocateInfo.space = GetBaseSpace();
  m_Views[0].type = XR_TYPE_VIEW;
  m_Views[1].type = XR_TYPE_VIEW;
  XrFovf previousFov[2];
  previousFov[0] = m_Views[0].fov;
  previousFov[1] = m_Views[1].fov;

  XrResult res = xrLocateViews(m_pSession, &viewLocateInfo, &m_ViewState, viewCapacityInput, &viewCountOutput, m_Views);

  if (res == XR_SUCCESS)
  {
    m_pInput->m_DeviceState[0].m_bGripPoseIsValid = ((m_ViewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) && (m_ViewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT));
    m_pInput->m_DeviceState[0].m_bAimPoseIsValid = m_pInput->m_DeviceState[0].m_bGripPoseIsValid;
  }
  else
  {
    m_pInput->m_DeviceState[0].m_bGripPoseIsValid = false;
    m_pInput->m_DeviceState[0].m_bAimPoseIsValid = false;
  }

  // Needed as workaround for broken XR runtimes.
  auto FovIsNull = [](const XrFovf& fov)
  {
    return fov.angleLeft == 0.0f && fov.angleRight == 0.0f && fov.angleDown == 0.0f && fov.angleUp == 0.0f;
  };

  auto IdentityFov = [](XrFovf& fov)
  {
    fov.angleLeft = -ezAngle::MakeFromDegree(45.0f).GetRadian();
    fov.angleRight = ezAngle::MakeFromDegree(45.0f).GetRadian();
    fov.angleUp = ezAngle::MakeFromDegree(45.0f).GetRadian();
    fov.angleDown = -ezAngle::MakeFromDegree(45.0f).GetRadian();
  };

  if (FovIsNull(m_Views[0].fov) || FovIsNull(m_Views[1].fov))
  {
    IdentityFov(m_Views[0].fov);
    IdentityFov(m_Views[1].fov);
  }

  m_bProjectionChanged = ezMemoryUtils::Compare(&previousFov[0], &m_Views[0].fov, 1) != 0 || ezMemoryUtils::Compare(&previousFov[1], &m_Views[1].fov, 1) != 0;

  for (ezUInt32 uiEyeIndex : {0, 1})
  {
    ezQuat rot = ConvertOrientation(m_Views[uiEyeIndex].pose.orientation);
    if (!rot.IsValid())
    {
      m_Views[uiEyeIndex].pose.orientation = XrQuaternionf{0, 0, 0, 1};
    }
  }

  UpdateCamera();
  m_pInput->UpdateActions();
  if (m_pHandTracking)
  {
    m_pHandTracking->UpdateJointTransforms();
  }
}

void ezOpenXR::UpdateCamera()
{
  if (!m_pCameraToSynchronize)
  {
    return;
  }
  // Update camera projection
  if (m_uiSettingsModificationCounter != m_pCameraToSynchronize->GetSettingsModificationCounter() || m_bProjectionChanged)
  {
    m_bProjectionChanged = false;
    const float fAspectRatio = (float)m_Info.m_vEyeRenderTargetSize.width / (float)m_Info.m_vEyeRenderTargetSize.height;
    auto CreateProjection = [](const XrView& view, ezCamera* cam)
    {
      return ezGraphicsUtils::CreatePerspectiveProjectionMatrix(ezMath::Tan(ezAngle::MakeFromRadian(view.fov.angleLeft)) * cam->GetNearPlane(), ezMath::Tan(ezAngle::MakeFromRadian(view.fov.angleRight)) * cam->GetNearPlane(), ezMath::Tan(ezAngle::MakeFromRadian(view.fov.angleDown)) * cam->GetNearPlane(),
        ezMath::Tan(ezAngle::MakeFromRadian(view.fov.angleUp)) * cam->GetNearPlane(), cam->GetNearPlane(), cam->GetFarPlane());
    };

    // Update projection with newest near/ far values. If not sync camera is set, just use the last value from XR
    // camera.
    const ezMat4 projLeft = CreateProjection(m_Views[0], m_pCameraToSynchronize);
    const ezMat4 projRight = CreateProjection(m_Views[1], m_pCameraToSynchronize);
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

    if (m_pInput->m_DeviceState[0].m_bGripPoseIsValid)
    {
      // Update device state (average of both eyes).
      const ezQuat rot = ezQuat::MakeSlerp(ConvertOrientation(m_Views[0].pose.orientation), ConvertOrientation(m_Views[1].pose.orientation), 0.5f);
      const ezVec3 pos = ezMath::Lerp(ConvertPosition(m_Views[0].pose.position), ConvertPosition(m_Views[1].pose.position), 0.5f);

      m_pInput->m_DeviceState[0].m_vGripPosition = pos;
      m_pInput->m_DeviceState[0].m_qGripRotation = rot;
      m_pInput->m_DeviceState[0].m_vAimPosition = pos;
      m_pInput->m_DeviceState[0].m_qAimRotation = rot;
      m_pInput->m_DeviceState[0].m_Type = ezXRDeviceType::HMD;
      m_pInput->m_DeviceState[0].m_bGripPoseIsValid = true;
      m_pInput->m_DeviceState[0].m_bAimPoseIsValid = true;
      m_pInput->m_DeviceState[0].m_bDeviceIsConnected = true;
    }

    // Set view matrix
    if (m_pInput->m_DeviceState[0].m_bGripPoseIsValid)
    {
      const ezMat4 mStageTransform = add.GetAsMat4();
      const ezMat4 poseLeft = mStageTransform * ConvertPoseToMatrix(m_Views[0].pose);
      const ezMat4 poseRight = mStageTransform * ConvertPoseToMatrix(m_Views[1].pose);

      // EZ Forward is +X, need to add this to align the forward projection
      const ezMat4 viewMatrix = ezGraphicsUtils::CreateLookAtViewMatrix(ezVec3::MakeZero(), ezVec3(1, 0, 0), ezVec3(0, 0, 1));
      const ezMat4 mViewTransformLeft = viewMatrix * poseLeft.GetInverse();
      const ezMat4 mViewTransformRight = viewMatrix * poseRight.GetInverse();

      m_pCameraToSynchronize->SetViewMatrix(mViewTransformLeft, ezCameraEye::Left);
      m_pCameraToSynchronize->SetViewMatrix(mViewTransformRight, ezCameraEye::Right);
    }
  }
}

void ezOpenXR::BeginFrame()
{
  if (m_hView.IsInvalidated() || !m_bSessionRunning)
    return;

  EZ_PROFILE_SCOPE("OpenXrBeginFrame");
  {
    EZ_PROFILE_SCOPE("xrWaitFrame");
    m_FrameWaitInfo = XrFrameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
    m_FrameState = XrFrameState{XR_TYPE_FRAME_STATE};
    XrResult result = xrWaitFrame(m_pSession, &m_FrameWaitInfo, &m_FrameState);
    if (result != XR_SUCCESS)
    {
      m_bRenderInProgress = false;
      return;
    }
  }
  {
    EZ_PROFILE_SCOPE("xrBeginFrame");
    m_FrameBeginInfo = XrFrameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};
    XrResult result = xrBeginFrame(m_pSession, &m_FrameBeginInfo);
    if (result != XR_SUCCESS)
    {
      m_bRenderInProgress = false;
      return;
    }
  }

  // #TODO_XR Swap chain acquire here?

  UpdatePoses();

  // This will update the extracted view from last frame with the new data we got
  // this frame just before starting to render.
  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView))
  {
    pView->UpdateViewData(ezRenderWorld::GetDataIndexForRendering());
  }

  if (m_pCompanion)
  {
    m_pCompanion->CompanionViewBeginFrame();
  }
  m_bRenderInProgress = true;
}

void ezOpenXR::DelayPresent()
{
  EZ_ASSERT_DEBUG(!m_bPresentDelayed, "Last present was not flushed");
  m_bPresentDelayed = true;
}

void ezOpenXR::Present()
{
  if (!m_bPresentDelayed)
    return;

  m_bPresentDelayed = false;

  const ezGALOpenXRSwapChain* pSwapChain = static_cast<const ezGALOpenXRSwapChain*>(ezGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain));
  if (!m_bRenderInProgress || !pSwapChain)
    return;

  if (m_pCompanion)
  {
    m_pCompanion->CompanionViewEndFrame();
    pSwapChain->PresentRenderTarget();
  }
}

ezGALTextureHandle ezOpenXR::GetCurrentTexture()
{
  const ezGALOpenXRSwapChain* pSwapChain = static_cast<const ezGALOpenXRSwapChain*>(ezGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain));
  if (!pSwapChain)
    return ezGALTextureHandle();

  return pSwapChain->m_hColorRT;
}

void ezOpenXR::EndFrame()
{
  const ezGALOpenXRSwapChain* pSwapChain = static_cast<const ezGALOpenXRSwapChain*>(ezGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain));

  if (!m_bRenderInProgress || !pSwapChain)
    return;
  /// NOTE: (Only Applies When Tracy is Enabled.)Tracy Seems to declare Timers in the same scope, so dual profile macros can throw: '__tracy_scoped_zone' : redefinition; multitple initalization, so we must scope the two events.
  {
    EZ_PROFILE_SCOPE("OpenXrEndFrame");
    for (uint32_t i = 0; i < 2; i++)
    {
      m_ProjectionLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW};
      m_ProjectionLayerViews[i].pose = m_Views[i].pose;
      m_ProjectionLayerViews[i].fov = m_Views[i].fov;
      m_ProjectionLayerViews[i].subImage.swapchain = pSwapChain->GetColorSwapchain();
      m_ProjectionLayerViews[i].subImage.imageRect.offset = {0, 0};
      m_ProjectionLayerViews[i].subImage.imageRect.extent = {(ezInt32)m_Info.m_vEyeRenderTargetSize.width, (ezInt32)m_Info.m_vEyeRenderTargetSize.height};
      m_ProjectionLayerViews[i].subImage.imageArrayIndex = i;

      if (m_Extensions.m_bDepthComposition && m_pCameraToSynchronize)
      {
        m_DepthLayerViews[i] = {XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR};
        m_DepthLayerViews[i].minDepth = 0;
        m_DepthLayerViews[i].maxDepth = 1;
        m_DepthLayerViews[i].nearZ = m_pCameraToSynchronize->GetNearPlane();
        m_DepthLayerViews[i].farZ = m_pCameraToSynchronize->GetFarPlane();
        m_DepthLayerViews[i].subImage.swapchain = pSwapChain->GetDepthSwapchain();
        m_DepthLayerViews[i].subImage.imageRect.offset = {0, 0};
        m_DepthLayerViews[i].subImage.imageRect.extent = {(ezInt32)m_Info.m_vEyeRenderTargetSize.width, (ezInt32)m_Info.m_vEyeRenderTargetSize.height};
        m_DepthLayerViews[i].subImage.imageArrayIndex = i;

        m_ProjectionLayerViews[i].next = &m_DepthLayerViews[i];
      }
    }
  }

  m_Layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
  m_Layer.space = GetBaseSpace();
  m_Layer.viewCount = 2;
  m_Layer.views = m_ProjectionLayerViews;

  ezHybridArray<XrCompositionLayerBaseHeader*, 1> layers;
  layers.PushBack(reinterpret_cast<XrCompositionLayerBaseHeader*>(&m_Layer));

  // Submit the composition layers for the predicted display time.
  XrFrameEndInfo frameEndInfo{XR_TYPE_FRAME_END_INFO};
  frameEndInfo.displayTime = m_FrameState.predictedDisplayTime;
  frameEndInfo.environmentBlendMode = m_BlendMode;
  frameEndInfo.layerCount = layers.GetCapacity();
  frameEndInfo.layers = layers.GetData();

  EZ_PROFILE_SCOPE("xrEndFrame");
  XR_LOG_ERROR(xrEndFrame(m_pSession, &frameEndInfo));
}

void ezOpenXR::GALDeviceEventHandler(const ezGALDeviceEvent& e)
{
  // Begin frame and end frame need to be encompassing all workload, XR and otherwise as xrWaitFrame will use this time interval to decide when to wake up the application.
  if (e.m_Type == ezGALDeviceEvent::Type::BeforeBeginFrame)
  {
    BeginFrame();
  }
  else if (e.m_Type == ezGALDeviceEvent::Type::BeforeEndFrame)
  {
    Present();
    EndFrame();
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

ezWorld* ezOpenXR::GetWorld()
{
  ezView* pView = nullptr;
  if (ezRenderWorld::TryGetView(m_hView, pView))
  {
    return pView->GetWorld();
  }
  return nullptr;
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
  return {q.y, q.z, -q.x, -q.w};
}

XrVector3f ezOpenXR::ConvertPosition(const ezVec3& vPos)
{
  return {vPos.y, vPos.z, -vPos.x};
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
