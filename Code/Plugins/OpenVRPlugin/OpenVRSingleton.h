#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/Interfaces/VRInterface.h>
#include <OpenVRPlugin/Basics.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

struct ezGameApplicationEvent;
typedef ezTypedResourceHandle<class ezShaderResource> ezShaderResourceHandle;

class EZ_OPENVRPLUGIN_DLL ezOpenVR : public ezVRInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezOpenVR, ezVRInterface);

public:
  ezOpenVR();

  virtual bool IsHmdPresent() const override;

  virtual bool Initialize() override;
  virtual void Deinitialize() override;
  virtual bool IsInitialized() const override;

  virtual const ezHMDInfo& GetHmdInfo() const override;
  virtual void GetDeviceList(ezHybridArray<ezVRDeviceID, 64>& out_Devices) const override;
  virtual ezVRDeviceID GetDeviceIDByType(ezVRDeviceType::Enum type) const override;
  virtual const ezVRDeviceState& GetDeviceState(ezVRDeviceID uiDeviceID) const override;
  virtual ezEvent<const ezVRDeviceEvent&>& DeviceEvents() override;

  virtual ezViewHandle CreateVRView(const ezRenderPipelineResourceHandle& hRenderPipeline, ezCamera* pCamera,
                                    ezGALMSAASampleCount::Enum msaaCount) override;
  virtual ezViewHandle GetVRView() const override;
  virtual bool DestroyVRView() override;
  virtual bool SupportsCompanionView() override;
  virtual bool SetCompanionViewRenderTarget(ezGALTextureHandle hRenderTarget) override;
  virtual ezGALTextureHandle GetCompanionViewRenderTarget() const override;

private:
  void GameApplicationEventHandler(const ezGameApplicationEvent& e);
  void GALDeviceEventHandler(const ezGALDeviceEvent& e);
  void OnBeginRender(ezUInt64);

  void ReadHMDInfo();
  void OnDeviceActivated(ezVRDeviceID uiDeviceID);
  void OnDeviceDeactivated(ezVRDeviceID uiDeviceID);

  void UpdatePoses();
  void UpdateHands();
  void SetStageSpace(ezVRStageSpace::Enum space);
  void SetHMDCamera(ezCamera* pCamera);
  void UpdateCamera();

  ezMat4 GetHMDProjectionEye(vr::Hmd_Eye nEye, float fNear, float fFar) const;
  ezMat4 GetHMDEyePose(vr::Hmd_Eye nEye) const;
  ezString GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop,
                                  vr::TrackedPropertyError* peError = nullptr) const;

  static ezMat4 ConvertSteamVRMatrix(const vr::HmdMatrix34_t& matPose);
  static ezVec3 ConvertSteamVRVector(const vr::HmdVector3_t& vector);

private:
  bool m_bInitialized = false;

  vr::IVRSystem* m_pHMD = nullptr;
  vr::IVRRenderModels* m_pRenderModels = nullptr;

  ezHMDInfo m_Info;
  ezVRDeviceState m_DeviceState[vr::k_unMaxTrackedDeviceCount];
  ezInt8 m_iLeftControllerDeviceID = -1;
  ezInt8 m_iRightControllerDeviceID = -1;
  ezEvent<const ezVRDeviceEvent&> m_DeviceEvents;

  ezWorld* m_pWorld = nullptr;
  ezCamera* m_pCameraToSynchronize = nullptr;
  ezEnum<ezVRStageSpace> m_StageSpace;

  ezCamera m_VRCamera;
  ezUInt32 m_uiSettingsModificationCounter = 0;
  ezViewHandle m_hView;
  ezGALRenderTagetSetup m_RenderTargetSetup;
  ezGALTextureCreationDescription m_eyeDesc;
  ezGALTextureHandle m_hColorRT;
  ezGALTextureHandle m_hDepthRT;

  ezGALTextureHandle m_hCompanionRenderTarget;
  ezConstantBufferStorageHandle m_hCompanionConstantBuffer;
  ezShaderResourceHandle m_hCompanionShader;
};

EZ_DYNAMIC_PLUGIN_DECLARATION(EZ_OPENVRPLUGIN_DLL, ezOpenVRPlugin);
