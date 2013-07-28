#pragma once

#include <Core/Input/InputDevice.h>

/// \brief This class is the base class for all controller type input devices.
///
/// This class is derived from ezInputDevice but adds some interface functions common to most controllers.
/// The device type name returned by GetDeviceType() is 'Controller'.
/// This class adds functions to query and modify the state about controller vibration, about the mapping of
/// physical controllers to virtual ones (which controller index triggers which controller input slots) and
/// also allows to query which controller is actually connected.
class EZ_CORE_DLL ezInputDeviceController : public ezInputDevice
{
public:
  
  /// \brief Returns 'Controller'.
  virtual const char* GetDeviceType() const { return "Controller"; }

  /// \brief Enables or disables vibration on the given controller (physical index).
  /// If it is disabled, the controller will never vibrate, even if vibration profiles are sent to it.
  virtual void EnableVibration(ezUInt8 uiController, bool bEnable) = 0;

  /// \brief Checks whether vibration is enabled on the given controller (physical index).
  /// If it is disabled, the controller will never vibrate, even if vibration profiles are sent to it.
  virtual bool IsVibrationEnabled(ezUInt8 uiController) const = 0;

  /// \brief Maps a physical controller index to a virtual controller index. This enables the controller that is connected
  /// at index 0 to trigger the input slots of controller 3. Several controllers may be mapped onto the same virtual
  /// controller, such that two or more controllers can activate the same actions.
  virtual void SetPhysicalToVirtualControllerMapping(ezUInt8 uiPhysical, ezInt8 iVirtual) = 0;

  /// \brief Returns which virtual controller is triggered by the given physical controller index.
  virtual ezInt8 GetPhysicalToVirtualControllerMapping(ezUInt8 uiPhysical) const = 0;

  /// \brief Queries whether the controller with the given physical index is connected to the computer.
  /// This may change at any time during game play.
  virtual bool IsControllerConnected(ezUInt8 uiPhysical) const = 0;
};