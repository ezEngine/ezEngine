#pragma once

#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

#include <GameEngine/XR/XRInterface.h>
#include <GameEngine/XR/XRInputDevice.h>

class ezOpenXR;

EZ_DEFINE_AS_POD_TYPE(XrActionSuggestedBinding);
EZ_DEFINE_AS_POD_TYPE(XrActiveActionSet);


class EZ_OPENXRPLUGIN_DLL ezOpenXRInputDevice : public ezXRInputDevice
{
  EZ_ADD_DYNAMIC_REFLECTION(ezOpenXRInputDevice, ezXRInputDevice);

public:

  ezString GetInteractionProfile() const override;

  void GetDeviceList(ezHybridArray<ezXRDeviceID, 64>& out_Devices) const override;
  ezXRDeviceID GetDeviceIDByType(ezXRDeviceType::Enum type) const override;
  const ezXRDeviceState& GetDeviceState(ezXRDeviceID iDeviceID) const override;

private:
  friend class ezOpenXR;
  struct Bind
  {
    XrAction action;
    const char* szPath;
  };

  struct Action
  {
    XrAction action;
    ezString sLeftKey;
    ezString sRightKey;
  };

  ezOpenXRInputDevice(ezOpenXR* pOpenXR);
  XrResult CreateActions(XrSession session, XrSpace m_sceneSpace);
  void DestroyActions();

  XrPath CreatePath(const char* szPath);
  XrResult CreateAction(const char* actionName, XrActionType actionType, XrAction& out_action);
  XrResult SuggestInteractionProfileBindings(const char* szInteractionProfile, ezArrayPtr<Bind> bindings);
  XrResult AttachSessionActionSets(XrSession session);

  void InitializeDevice() override;
  void RegisterInputSlots() override;
  void UpdateInputSlotValues() override {}

  XrResult UpdateActions();

private:
  ezOpenXR* m_pOpenXR = nullptr;
  XrInstance m_instance = XR_NULL_HANDLE;
  XrSession m_session = XR_NULL_HANDLE;

  ezXRDeviceState m_DeviceState[3]; // Hard-coded for now
  const ezInt8 m_iLeftControllerDeviceID = 1;
  const ezInt8 m_iRightControllerDeviceID = 2;

  XrActionSet m_ActionSet = XR_NULL_HANDLE;

  ezStaticArray<const char*, 2> m_subActionPrefix;
  ezStaticArray<XrPath, 2> m_subActionPath;

  ezHybridArray<Action, 4> m_booleanActions;
  ezHybridArray<Action, 4> m_poseActions;


  XrAction m_SelectClick = XR_NULL_HANDLE;
  XrAction m_MenuClick = XR_NULL_HANDLE;
  XrAction m_GripPose = XR_NULL_HANDLE;
  XrSpace m_gripSpace[2];
  XrAction m_AimPose = XR_NULL_HANDLE;
  XrSpace m_aimSpace[2];


};
