#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/XRInputDevice.h>
#include <GameEngine/XR/XRInterface.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class ezOpenXRInputDevice;
class ezOpenXRSpatialAnchors;
class ezOpenXRHandTracking;
class ezOpenXRGraphicsBinding;
class ezWindowOutputTargetXR;
struct ezGameApplicationExecutionEvent;
struct ezRenderWorldRenderEvent;
class ezGALDevice;

EZ_DEFINE_AS_POD_TYPE(XrViewConfigurationView);
EZ_DEFINE_AS_POD_TYPE(XrEnvironmentBlendMode);
EZ_DEFINE_AS_POD_TYPE(XrExtensionProperties);
EZ_DEFINE_AS_POD_TYPE(XrApiLayerProperties);

class EZ_OPENXRPLUGIN_DLL ezOpenXR : public ezXRInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezOpenXR, ezXRInterface);

public:
  ezOpenXR();
  ~ezOpenXR();

  XrInstance GetInstance() const { return m_pInstance; }
  uint64_t GetSystemId() const { return m_SystemId; }
  XrSession GetSession() const { return m_pSession; }
  XrViewConfigurationType GetViewType() const { return m_PrimaryViewConfigurationType; }
  bool GetDepthComposition() const;

  /// \brief Returns the graphics binding interface (D3D11, Vulkan, etc.)
  ezOpenXRGraphicsBinding* GetGraphicsBinding() const { return m_pGraphicsBinding.Borrow(); }

  virtual bool IsHmdPresent() const override;

  virtual ezResult Initialize() override;
  virtual void Deinitialize() override;
  virtual bool IsInitialized() const override;

  virtual const ezHMDInfo& GetHmdInfo() const override;
  virtual ezXRInputDevice& GetXRInput() const override;

  virtual ezGALTextureHandle GetCurrentTexture() override;

  virtual ezRegisteredWndHandle CreateXRWindow(ezView* pView, ezGALMSAASampleCount::Enum msaaCount = ezGALMSAASampleCount::None,
    ezUniquePtr<ezWindowBase> pCompanionWindow = nullptr, ezUniquePtr<ezWindowOutputTargetGAL> pCompanionWindowOutput = nullptr) override;
  virtual void OnActorDestroyed() override;
  virtual bool SupportsCompanionView() override;

  XrSpace GetBaseSpace() const;

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererFoundation, OpenXR);

  void OnEngineStartup();
  void OnEngineShutdown();
  XrResult SelectExtensions(ezHybridArray<const char*, 6>& extensions);
  XrResult SelectLayers(ezHybridArray<const char*, 6>& layers);
  ezResult InitInstance(ezGALDevice* pDevice);
  void DeinitInstance();
  XrResult InitSystem();
  void DeinitSystem();
  XrResult InitSession();
  void DeinitSession();
  XrResult InitGraphicsPlugin();
  void DeinitGraphicsPlugin();
  XrResult InitDebugMessenger();
  void DeinitInitDebugMessenger();

  void GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e);
  void GALDeviceEventHandler(const ezGALDeviceEvent& e);
  void OnRenderWorldEvent(const ezRenderWorldRenderEvent& e);

  void BeforeUpdatePlugins();
  void UpdatePoses();
  void UpdateCamera();
  void BeginFrame();
  void EndRender();
  void EndFrame();

  void SetStageSpace(ezXRStageSpace::Enum space);
  void SetHMDCamera(ezCamera* pCamera);

  ezWorld* GetWorld();

private:
  friend class ezOpenXRInputDevice;
  friend class ezOpenXRSpatialAnchors;
  friend class ezOpenXRHandTracking;
  friend class ezOpenXRRemoting;
  friend class ezGALOpenXRSwapChain;

  struct Extensions
  {
    bool m_bValidation = false;
    bool m_bDebugUtils = false;
    PFN_xrCreateDebugUtilsMessengerEXT pfn_xrCreateDebugUtilsMessengerEXT = nullptr;
    PFN_xrDestroyDebugUtilsMessengerEXT pfn_xrDestroyDebugUtilsMessengerEXT = nullptr;

    bool m_bDepthComposition = false;

    bool m_bUnboundedReferenceSpace = false;

    bool m_bSpatialAnchor = false;
    PFN_xrCreateSpatialAnchorMSFT pfn_xrCreateSpatialAnchorMSFT = nullptr;
    PFN_xrCreateSpatialAnchorSpaceMSFT pfn_xrCreateSpatialAnchorSpaceMSFT = nullptr;
    PFN_xrDestroySpatialAnchorMSFT pfn_xrDestroySpatialAnchorMSFT = nullptr;

    bool m_bHandInteraction = false;

    bool m_bHandTracking = false;
    PFN_xrCreateHandTrackerEXT pfn_xrCreateHandTrackerEXT = nullptr;
    PFN_xrDestroyHandTrackerEXT pfn_xrDestroyHandTrackerEXT = nullptr;
    PFN_xrLocateHandJointsEXT pfn_xrLocateHandJointsEXT = nullptr;

    bool m_bHandTrackingMesh = false;
    PFN_xrCreateHandMeshSpaceMSFT pfn_xrCreateHandMeshSpaceMSFT = nullptr;
    PFN_xrUpdateHandMeshMSFT pfn_xrUpdateHandMeshMSFT = nullptr;

    bool m_bHolographicWindowAttachment = false;
  };

  // Instance
  XrInstance m_pInstance = XR_NULL_HANDLE;
  Extensions m_Extensions;

  // System
  uint64_t m_SystemId = XR_NULL_SYSTEM_ID;

  // Session
  XrSession m_pSession = XR_NULL_HANDLE;
  XrSpace m_pSceneSpace = XR_NULL_HANDLE;
  XrSpace m_pLocalSpace = XR_NULL_HANDLE;
  ezEventSubscriptionID m_ExecutionEventsId = 0;
  ezEventSubscriptionID m_BeginRenderEventsId = 0;
  ezEventSubscriptionID m_GALdeviceEventsId = 0;
  ezEventSubscriptionID m_RenderWorldEventId = 0;
  XrDebugUtilsMessengerEXT m_pDebugMessenger = XR_NULL_HANDLE;

  // Graphics binding (abstracts D3D11, Vulkan, etc.)
  XrEnvironmentBlendMode m_BlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  ezUniquePtr<ezOpenXRGraphicsBinding> m_pGraphicsBinding;
  XrFormFactor m_FormFactor{XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};
  XrViewConfigurationType m_PrimaryViewConfigurationType{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};

  ezGALSwapChainHandle m_hSwapChain;

  // Views
  XrViewState m_ViewState{XR_TYPE_VIEW_STATE};
  XrView m_Views[2];
  bool m_bProjectionChanged = true;
  XrCompositionLayerProjection m_Layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  XrCompositionLayerProjectionView m_ProjectionLayerViews[2];
  XrCompositionLayerDepthInfoKHR m_DepthLayerViews[2];

  // State
  bool m_bSessionRunning = false;
  bool m_bExitRenderLoop = false;
  bool m_bRequestRestart = false;
  bool m_bRenderInProgress = false;
  XrSessionState m_SessionState{XR_SESSION_STATE_UNKNOWN};

  XrFrameWaitInfo m_FrameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
  XrFrameState m_FrameState{XR_TYPE_FRAME_STATE};
  XrFrameBeginInfo m_FrameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};

  // XR interface state
  ezHMDInfo m_Info;
  mutable ezUniquePtr<ezOpenXRInputDevice> m_pInput;
  ezUniquePtr<ezOpenXRSpatialAnchors> m_pAnchors;
  ezUniquePtr<ezOpenXRHandTracking> m_pHandTracking;

  ezCamera* m_pCameraToSynchronize = nullptr;
  ezEnum<ezXRStageSpace> m_StageSpace;
  ezUInt32 m_uiSettingsModificationCounter = 0;
  ezViewHandle m_hView;

  ezWindowOutputTargetXR* m_pCompanion = nullptr;
};
