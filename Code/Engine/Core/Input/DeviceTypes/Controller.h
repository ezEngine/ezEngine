#pragma once

#include <Core/Input/InputDevice.h>

/// \brief This class is the base class for all controller type input devices.
///
/// This class is derived from ezInputDevice but adds some interface functions common to most controllers.
/// This class adds functions to query and modify the state about controller vibration, about the mapping of
/// physical controllers to virtual ones (which controller index triggers which controller input slots) and
/// also allows to query which controller is actually connected.
class EZ_CORE_DLL ezInputDeviceController : public ezInputDevice
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInputDeviceController);

public:
  enum 
  { 
    MaxControllers = 4,
    VibrationSamplesPerSecond = 16,
    VibrationTrackSeconds = 2,
    MaxVibrationSamples = VibrationSamplesPerSecond * VibrationTrackSeconds, // With constant power-of-two samples some code should get more efficient
  };

  /// \brief Describes which vibration motor to configure.
  struct Motor
  {
    enum Enum
    {
      LeftMotor,
      RightMotor,
      ENUM_COUNT
    };
  };

  ezInputDeviceController();
  
  /// \brief Enables or disables vibration on the given controller (virtual index).
  /// If it is disabled, the controller will never vibrate, even if vibration profiles are sent to it.
  void EnableVibration(ezUInt8 uiVirtual, bool bEnable);

  /// \brief Checks whether vibration is enabled on the given controller (virtual index).
  bool IsVibrationEnabled(ezUInt8 uiVirtual) const;

  /// \brief Sets the vibration strength for the given controller and motor. \a fValue is a value between 0 and 1.
  ///
  /// From now on the controller will be vibrating (unless vibration is disabled), until the value is reset to zero.
  /// This kind of vibration is always combined with vibration tracks (the maximum of both values is applied at any
  /// one time). Using this function is it possible to have more direct control over vibration, while the
  /// vibration tracks are convenient for the most common (short) effects.
  void SetVibrationStrength(ezUInt8 uiVirtual, Motor::Enum eMotor, float fValue);

  /// \brief Returns the amount of (constant) vibration that is currently set on this controller.
  float GetVibrationStrength(ezUInt8 uiVirtual, Motor::Enum eMotor);

  /// \brief Sets from which physical controller a virtual controller is supposed to take its input.
  ///
  /// If iTakeInputFromPhysical is smaller than zero, the given virtual controller is deactivated (it will generate no input).
  /// If input is taken from a physical controller, that is already mapped to another virtual controller, that virtual controller
  /// will now take input from the physical controller that uiVirtualController was previously mapped to (ie. they will swap
  /// from which physical controller to take input).
  /// By default all virtual controllers take their input from the physical controller with the same index.
  /// You can use this feature to let the player pick up any controller, detect which one it is (e.g. by forcing him to press 'Start')
  /// and then map that physical controller index to the virtual index 0 (ie. player 1).
  /// Note that unless you specify a negative index for a mapping (which deactivates that virtual controller), mapping controllers
  /// around does never deactivate any controller, because the indices are swapped between the different virtual controllers,
  /// so which physical controller maps to which virtual controller only 'moves around'.
  void SetControllerMapping(ezUInt8 uiVirtualController, ezInt8 iTakeInputFromPhysical);

  /// \brief Returns from which physical controller the given virtual controller takes its input. May be negative, which means
  /// the virtual controller is deactivated.
  ezInt8 GetControllerMapping(ezUInt8 uiVirtual) const;

  /// \brief Queries whether the controller with the given physical index is connected to the computer.
  /// This may change at any time.
  virtual bool IsControllerConnected(ezUInt8 uiPhysical) const = 0;

  /// \brief Adds a short 'vibration track' (a sequence of vibrations) to the given controller.
  ///
  /// Each controller has a short (typically 2 second) buffer for vibration values, that it will play.
  /// This allows to have different 'tracks' for different events, which are simply set on the controller.
  /// You can add an unlimited amount of tracks on a controller, the controller stores the maximum of all tracks
  /// and plays that.
  /// That means whenever the player shoots, or is hit etc., you can add a vibration track to the controller
  /// and it will be combined with all other tracks and played (no memory allocations are required).
  ///
  /// \param uiVirtual The virtual index of the controller.
  /// \param eMotor Which motor to apply the track on.
  /// \param fVibrationTrackValue An array of at least \a uiSamples float values, each between 0 and 1.
  /// \param uiSamples How many samples \a fVibrationTrackValue contains. A maximum of MaxVibrationSamples samples is used.
  /// \param fScalingFactor Additional scaling factor to apply to all values in \a fVibrationTrackValue.
  void AddVibrationTrack(ezUInt8 uiVirtual, Motor::Enum eMotor, float* fVibrationTrackValue, ezUInt32 uiSamples, float fScalingFactor = 1.0f);

protected:
  /// \brief Combines the constant vibration and vibration tracks and applies them on each controller.
  ///
  /// This function needs to be called by a derived implementation in its UpdateInputSlotValues() function.
  /// It will call ApplyVibration() for each controller and motor with the current value. It already takes care
  /// of whether vibration is enabled or disabled, and also mapping virtual to physical controllers.
  void UpdateVibration(ezTime tTimeDifference);

private:
  /// \brief Must be implemented by a derived controller implementation. Should set apply the vibration for the given physical controller and motor with the given strength.
  ///
  /// A strength value of zero will be passed in whenever no vibration is required. No extra resetting needs to be implemented.
  virtual void ApplyVibration(ezUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength) = 0;

  ezUInt32 m_uiVibrationTrackPos;
  float m_fVibrationTracks[MaxControllers][Motor::ENUM_COUNT][MaxVibrationSamples];
  bool m_bVibrationEnabled[MaxControllers];
  ezInt8 m_iControllerMapping[MaxControllers];
  float m_fVibrationStrength[MaxControllers][Motor::ENUM_COUNT];

};

