#pragma once

#include <OpenVRPlugin/Basics.h>
#include <GameEngine/Interfaces/VRInterface.h>
#include <Foundation/Configuration/Singleton.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

struct ezGameApplicationEvent;

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
  virtual void GetDeviceList(ezHybridArray<ezUInt8, 64>& out_Devices) const override;
  virtual const ezVRDeviceState& GetDeviceState(ezUInt8 uiDeviceID) const override;
  virtual ezEvent<const ezVRDeviceEvent&>& DeviceEvents() override;

  virtual ezViewHandle CreateVRView(const ezRenderPipelineResourceHandle& hRenderPipeline, ezCamera* pCamera,
                                             ezGALMSAASampleCount::Enum msaaCount) override;
  virtual ezViewHandle GetVRView() const override;
  virtual bool DestroyVRView() override;

private:
  void GameApplicationEventHandler(const ezGameApplicationEvent& e);

  void ReadHMDInfo();
  void OnDeviceActivated(ezUInt8 uiDeviceID);
  void OnDeviceDeactivated(ezUInt8 uiDeviceID);

  void UpdatePoses();
  void SetHMDCamera(ezCamera* pCamera);

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
  ezEvent<const ezVRDeviceEvent&> m_DeviceEvents;

  ezCamera* m_pCameraToSynchronize = nullptr;
  ezTransform m_AdditionalCameraTransform;

  ezViewHandle m_hView;
  ezGALRenderTagetSetup m_RenderTargetSetup;
  ezGALTextureCreationDescription m_eyeDesc;
  ezGALTextureHandle m_hColorRT;
  ezGALTextureHandle m_hDepthRT;
};

EZ_DYNAMIC_PLUGIN_DECLARATION(EZ_OPENVRPLUGIN_DLL, ezOpenVRPlugin);
