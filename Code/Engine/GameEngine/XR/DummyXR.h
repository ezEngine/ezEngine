#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <GameEngine/XR/Declarations.h>
#include <GameEngine/XR/XRInputDevice.h>
#include <GameEngine/XR/XRInterface.h>
#include <RendererCore/Pipeline/Declarations.h>

struct ezGALDeviceEvent;
struct ezGameApplicationExecutionEvent;
class ezWindowOutputTargetXR;

class EZ_GAMEENGINE_DLL ezDummyXRInput : public ezXRInputDevice
{

public:
  void GetDeviceList(ezHybridArray<ezXRDeviceID, 64>& out_Devices) const override;
  ezXRDeviceID GetDeviceIDByType(ezXRDeviceType::Enum type) const override;
  const ezXRDeviceState& GetDeviceState(ezXRDeviceID iDeviceID) const override;
  ezString GetDeviceName(ezXRDeviceID iDeviceID) const override;
  ezBitflags<ezXRDeviceFeatures> GetDeviceFeatures(ezXRDeviceID iDeviceID) const override;

protected:
  void InitializeDevice() override;
  void UpdateInputSlotValues() override;
  void RegisterInputSlots() override;

protected:
  friend class ezDummyXR;

  ezXRDeviceState m_DeviceState[1];
};

class EZ_GAMEENGINE_DLL ezDummyXR : public ezXRInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezDummyXR, ezXRInterface);

public:
  ezDummyXR();
  ~ezDummyXR();

  bool IsHmdPresent() const override;
  ezResult Initialize() override;
  void Deinitialize() override;
  bool IsInitialized() const override;
  const ezHMDInfo& GetHmdInfo() const override;
  ezXRInputDevice& GetXRInput() const override;
  bool SupportsCompanionView() override;
  ezUniquePtr<ezActor> CreateActor(ezView* pView, ezGALMSAASampleCount::Enum msaaCount = ezGALMSAASampleCount::None, ezUniquePtr<ezWindowBase> companionWindow = nullptr, ezUniquePtr<ezWindowOutputTargetGAL> companionWindowOutput = nullptr) override;
  ezGALTextureHandle GetCurrentTexture() override;
  void OnActorDestroyed() override;
  void GALDeviceEventHandler(const ezGALDeviceEvent& e);
  void GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e);

protected:
  float m_fHeadHeight = 1.7f;
  float m_fEyeOffset = 0.05f;


  ezHMDInfo m_Info;
  mutable ezDummyXRInput m_Input;
  bool m_bInitialized = false;

  ezEventSubscriptionID m_GALdeviceEventsId = 0;
  ezEventSubscriptionID m_ExecutionEventsId = 0;

  ezWorld* m_pWorld = nullptr;
  ezCamera* m_pCameraToSynchronize = nullptr;
  ezEnum<ezXRStageSpace> m_StageSpace = ezXRStageSpace::Seated;

  ezViewHandle m_hView;
  ezGALTextureHandle m_hColorRT;
  ezGALTextureHandle m_hDepthRT;

  ezWindowOutputTargetXR* m_pCompanion = nullptr;
};
