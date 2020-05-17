#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <Core/Input/InputDevice.h>
#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/Declarations.h>

#define ezInputSlot_XR_Hand_Left_Select_Click "xr_hand_left_select_click"
#define ezInputSlot_XR_Hand_Left_Menu_Click "xr_hand_left_menu_click"
#define ezInputSlot_XR_Hand_Left_Grip_Pose "xr_hand_left_grip_pose"
#define ezInputSlot_XR_Hand_Left_Aim_Pose "xr_hand_left_aim_pose"


#define ezInputSlot_XR_Hand_Right_Select_Click "xr_hand_right_select_click"
#define ezInputSlot_XR_Hand_Right_Menu_Click "xr_hand_right_menu_click"
#define ezInputSlot_XR_Hand_Right_Grip_Pose "xr_hand_right_grip_pose"
#define ezInputSlot_XR_Hand_Right_Aim_Pose "xr_hand_right_aim_pose"


#define XR_Select_Click "select_click"
#define XR_Menu_Click "menu_click"
#define XR_Grip_Pose "grip_pose"
#define XR_Aim_Pose "aim_pose"


class EZ_GAMEENGINE_DLL ezXRInputDevice : public ezInputDevice
{
  EZ_ADD_DYNAMIC_REFLECTION(ezXRInputDevice, ezInputDevice);

public:
  virtual ezString GetInteractionProfile() const = 0;

  /// \name Devices
  ///@{

  /// \brief Fills out a list of valid (connected) device IDs.
  virtual void GetDeviceList(ezHybridArray<ezXRDeviceID, 64>& out_Devices) const = 0;
  /// \brief Returns the deviceID for a specific type of device.
  /// If the device is not connected, -1 is returned instead.
  virtual ezXRDeviceID GetDeviceIDByType(ezXRDeviceType::Enum type) const = 0;
  /// \brief Returns the current device state for a valid device ID.
  virtual const ezXRDeviceState& GetDeviceState(ezXRDeviceID iDeviceID) const = 0;

  /// \brief Returns the input event. Allows tracking device addition and removal.
  const ezXRDeviceEvent& GetInputEvent() { return m_InputEvents; }

  ///@}

protected:
  ezXRDeviceEvent m_InputEvents;
};
