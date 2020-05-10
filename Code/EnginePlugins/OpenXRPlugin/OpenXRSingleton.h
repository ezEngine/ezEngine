#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/XRInterface.h>
#include <GameEngine/XR/XRInputDevice.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Device/SwapChain.h>

class ezOpenXRInputDevice;

struct ezGameApplicationExecutionEvent;

EZ_DEFINE_AS_POD_TYPE(XrSwapchainImageD3D11KHR);
EZ_DEFINE_AS_POD_TYPE(XrViewConfigurationView);
EZ_DEFINE_AS_POD_TYPE(XrEnvironmentBlendMode);

//#TODO: Make use of this to abstract render api.
class EZ_OPENXRPLUGIN_DLL ezGALOpenXRSwapChainDX11 : public ezGALSwapChain
{
public:
  ezGALOpenXRSwapChainDX11(const ezGALSwapChainCreationDescription& Description);

  virtual ~ezGALOpenXRSwapChainDX11();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;

  void SetSwapchainTexture(ezGALTextureHandle hBackBufferTexture);

protected:
};

class EZ_OPENXRPLUGIN_DLL ezOpenXR : public ezXRInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezOpenXR, ezXRInterface);

public:
  ezOpenXR();
  ~ezOpenXR();

  virtual bool IsHmdPresent() const override;

  virtual ezResult Initialize() override;
  virtual void Deinitialize() override;
  virtual bool IsInitialized() const override;

  virtual const ezHMDInfo& GetHmdInfo() const override;
  virtual ezXRInputDevice& GetXRInput() const override;

  virtual ezGALTextureHandle Present() override;
  virtual ezUniquePtr<ezActor> CreateActor(
    ezView* pView, ezGALMSAASampleCount::Enum msaaCount = ezGALMSAASampleCount::None,
    ezUniquePtr<ezWindowBase> companionWindow = nullptr, ezUniquePtr<ezWindowOutputTargetBase> companionWindowOutput = nullptr) override;
  virtual void OnActorDestroyed() override;
  virtual bool SupportsCompanionView() override;

private:
  struct Swapchain
  {
    XrSwapchain handle = 0;
    int64_t format = 0;
    ezUInt32 imageCount = 0;
    XrSwapchainImageBaseHeader* images = nullptr;
    uint32_t imageIndex = 0;
  };
  enum class SwapchainType
  {
    Color,
    Depth,
  };

  ezResult SelectExtensions(ezHybridArray<const char*, 6>& extensions);
  XrResult InitSystem();
  void DeinitSystem();
  XrResult InitSession();
  void DeinitSession();
  XrResult InitGraphicsPlugin();
  void DeinitGraphicsPlugin();
  XrResult SelectSwapchainFormat(int64_t& colorFormat, int64_t& depthFormat);
  XrResult CreateSwapchainImages(Swapchain& swapchain, SwapchainType type);
  XrResult InitSwapChain(ezGALMSAASampleCount::Enum msaaCount);
  void DeinitSwapChain();

  void GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e);
  void GALDeviceEventHandler(const ezGALDeviceEvent& e);

  void BeforeUpdatePlugins();
  void UpdatePoses();
  void UpdateCamera();
  void BeforeBeginFrame();

  void SetStageSpace(ezXRStageSpace::Enum space);
  void SetHMDCamera(ezCamera* pCamera);

  static XrPosef ConvertTransform(const ezTransform& tr);
  static XrQuaternionf ConvertOrientation(const ezQuat& q);
  static XrVector3f ConvertPosition(const ezVec3& pos);
  static ezQuat ConvertOrientation(const XrQuaternionf& q);
  static ezVec3 ConvertPosition(const XrVector3f& pos);
  static ezMat4 ConvertPoseToMatrix(const XrPosef& pose);
  static ezGALResourceFormat::Enum ConvertTextureFormat(int64_t format);
  static ezGALMSAASampleCount::Enum ConvertMSAACount(ezUInt32 count);

private:
  friend class ezOpenXRInputDevice;

  struct Extensions
  {
    bool m_bD3D11 = false;
    PFN_xrGetD3D11GraphicsRequirementsKHR pfn_xrGetD3D11GraphicsRequirementsKHR;
    
    bool m_bDepthComposition = false;

    bool m_bUnboundedReferenceSpace = false;

    bool m_bSpatialAnchor = false;
    PFN_xrCreateSpatialAnchorMSFT pfn_xrCreateSpatialAnchorMSFT;
    PFN_xrCreateSpatialAnchorSpaceMSFT pfn_xrCreateSpatialAnchorSpaceMSFT;
    PFN_xrDestroySpatialAnchorMSFT pfn_xrDestroySpatialAnchorMSFT;

    bool m_bHandInteraction = false;

    bool m_bHandTracking = false;
#ifdef BUILDSYSTEM_ENABLE_OPENXR_PREVIEW_SUPPORT
    PFN_xrCreateHandTrackerMSFT pfn_xrCreateHandTrackerMSFT;
    PFN_xrDestroyHandTrackerMSFT pfn_xrDestroyHandTrackerMSFT;
    PFN_xrGetHandTrackerStateMSFT pfn_xrGetHandTrackerStateMSFT;
    PFN_xrCreateHandJointSpaceMSFT pfn_xrCreateHandJointSpaceMSFT;
#endif
    bool m_bHandTrackingMesh = false;
#ifdef BUILDSYSTEM_ENABLE_OPENXR_PREVIEW_SUPPORT
    PFN_xrCreateHandMeshSpaceMSFT pfn_xrCreateHandMeshSpaceMSFT;
    PFN_xrUpdateHandMeshMSFT pfn_xrUpdateHandMeshMSFT;
#endif
  };

  // Instance
  XrInstance m_instance = XR_NULL_HANDLE;
  Extensions m_extensions;

  // System
  uint64_t m_systemId = XR_NULL_SYSTEM_ID;

  // Session
  XrSession m_session = XR_NULL_HANDLE;
  XrSpace m_sceneSpace = XR_NULL_HANDLE;
  XrSpace m_localSpace = XR_NULL_HANDLE;
  ezEventSubscriptionID m_executionEventsId = 0;
  ezEventSubscriptionID m_beginRenderEventsId = 0;
  ezEventSubscriptionID m_GALdeviceEventsId = 0;

  // Graphics plugin
  XrEnvironmentBlendMode m_blendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  XrGraphicsBindingD3D11KHR m_xrGraphicsBindingD3D11{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
  XrFormFactor m_formFactor{XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};
  XrViewConfigurationType m_primaryViewConfigurationType{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
  ezHybridArray<XrSwapchainImageD3D11KHR, 3> m_colorSwapChainImagesD3D11;
  ezHybridArray<XrSwapchainImageD3D11KHR, 3> m_depthSwapChainImagesD3D11;

  // Swapchain
  XrViewConfigurationView m_primaryConfigView;
  Swapchain m_colorSwapchain;
  Swapchain m_depthSwapchain;

  // Views
  XrView m_views[2];
  bool m_projectionChanged = true;
  XrCompositionLayerProjection m_layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  XrCompositionLayerProjectionView m_projectionLayerViews[2];
  XrCompositionLayerDepthInfoKHR m_depthLayerViews[2];

  // State
  bool m_sessionRunning = false;
  bool m_exitRenderLoop = false;
  bool m_requestRestart = false;
  bool m_renderInProgress = false;
  XrSessionState m_sessionState{XR_SESSION_STATE_UNKNOWN};

  ezInt32 m_updateFrame = 0;
  ezInt32 m_renderFrame = 1;
  XrFrameWaitInfo m_frameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
  XrFrameState m_frameState{XR_TYPE_FRAME_STATE};
  XrFrameBeginInfo m_frameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};

  // XR interface state
  ezHMDInfo m_Info;
  mutable ezUniquePtr<ezOpenXRInputDevice> m_Input;

  ezWorld* m_pWorld = nullptr;
  ezCamera* m_pCameraToSynchronize = nullptr;
  ezEnum<ezXRStageSpace> m_StageSpace;

  ezUInt32 m_uiSettingsModificationCounter = 0;
  ezViewHandle m_hView;
  ezGALRenderTargetSetup m_RenderTargetSetup;
  ezGALTextureHandle m_hColorRT;
  ezGALTextureHandle m_hDepthRT;


};
