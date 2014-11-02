#pragma once

#include <Core/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Reflection/Reflection.h>
#include <Core/Input/Declarations.h>

/// \brief The base class for all input device types.
///
/// An input device is the abstraction of one or more types of input. It is not linked to one physical device.
/// For example an input device can represent mouse AND keyboard (through one class). Another input device
/// can represent all connected controllers (e.g. up to 4 XBox 360 controllers).
/// On OSes where applications can have several windows, an input device may also represent all input from one window, if required.
///
/// An input device is the abstraction layer for the hardware input. All keys, movements etc. are mapped to named 'input slots',
/// which typically hold a value between 0 and 1, where 0 represents 'not pressed' and 1 represents 'fully pressed'. All hardware
/// that has a finite range (such as buttons, analog triggers, the positive/negative axis of analog sticks) should try to map to this
/// range. Even mouse coordinates are typically mapped to the 0 to 1 range, where zero means top/left and 1 means bottom/right.
///
/// All input handling is usually handled through the ezInputManager class. 
/// A user should typically not have to interact directly with an input device, unless he needs to call device specific functions
/// for advanced configuration.
///
/// An input device defines which input slots it exposes to the engine. All input slots are handled by name (e.g. string).
/// For example a keyboard would expose the input slots 'keyboard_a' to 'keyboard_z' and other keys. A mouse would expose slots
/// such as 'mouse_move_pos_x' and 'mouse_move_neg_x' etc.
///
/// By deriving from ezInputDevice you can extend what hardware the engine supports. The derived class should override
/// InitializeDevice() to do hardware specific setup. It also needs to override RegisterInputSlots() to register all the input slots
/// that it wants to expose from the hardware. E.g. if the device wants to expose values from a gyroscope, it should register
/// input slots that represent the rotations around the different axis (one slot each for both positive and negative changes).
/// It then also needs to implement UpdateInputSlotValues() and/or ResetInputSlotValues() and possible a device-specific update
/// function, to get the input values. For example on Windows a platform/device specific update function is necessary to parse
/// the incoming window messages. If such a device specific function is necessary, it also needs to be integrated into the proper
/// code (e.g. into the window handling code, to be able to get the window messages). In such a case it might not be possible
/// to add such a device purely through a dynamic plugin, but might also need deeper integration into other engine code.
class EZ_CORE_DLL ezInputDevice : public ezEnumerable<ezInputDevice, ezReflectedClass>
{
  EZ_DECLARE_ENUMERABLE_CLASS_WITH_BASE(ezInputDevice, ezReflectedClass);
  EZ_ADD_DYNAMIC_REFLECTION(ezInputDevice);

public:
  /// \brief Default Constructor.
  ezInputDevice();

private:
  friend class ezInputManager;

  /// \brief If this type of input device handles character input (typed text with all its formatting), this function returns the last typed character.
  ///
  /// An input device that handles keyboard input should also have a way to query the real typed character. I.e. by default only the
  /// individual state of each key is handled, such that we know that the shift key and the a key are pressed. However, the fact that
  /// this results in an upper case A in typed text also needs to be handled. An OS usually has a way to compute this, for example
  /// on Windows the WM_CHAR message sends this information. An ezInputDevice derived class should never try to compute this
  /// itself, but instead query this information from the OS, which will also handle localization.
  ezUInt32 RetrieveLastCharacter();

  /// \brief Calls UpdateHardwareState() on all devices.
  static void UpdateAllHardwareStates(ezTime tTimeDifference);

  /// \brief Calls Initialize() and UpdateInputSlotValues() on all devices.
  static void UpdateAllDevices();

  /// \brief Calls ResetInputSlotValues() on all devices.
  static void ResetAllDevices();

  /// \brief Calls RetrieveLastCharacter() on all devices. Returns the first non-null character that any device returned.
  static ezUInt32 RetrieveLastCharacterFromAllDevices();

protected:
  /// \brief Calls RegisterInputSlot() on the ezInputManager and passes the parameters through.
  static void RegisterInputSlot(const char* szName, const char* szDefaultDisplayName, ezBitflags<ezInputSlotFlags> SlotFlags); // [tested]

  /// \brief Stores all the values for all input slots that this device handles.
  ///
  /// A derived class needs to fill out this map every frame. There are two ways this map can be filled out.
  /// For devices where you can query the complete state at one point in time (e.g. controllers), you can update the entire
  /// map inside an overridden UpdateInputSlotValues() function.
  /// For devices where you get the input only piece-wise and usually only when something changes (e.g. through messages)
  /// you can also just update the map whenever input arrives. However in such a use-case you sometimes need to manually
  /// reset the state of certain input slots. For example when a mouse-move message arrives that movement delta is accumulated in
  /// the map. However, when the mouse stops usually no 'mouse stopped' message is sent but the values in the map need to be
  /// reset to zero, to prevent the mouse from keeping moving in the engine.
  /// Do this inside an overridden ResetInputSlotValues() function. You don't need to do this for input slots that
  /// will reset to zero anyway.
  ezMap<ezString, float> m_InputSlotValues; // [tested]

  /// \brief If this input device type handles character input, it should write the last typed character into this variable.
  /// The ezInputManager calls RetrieveLastCharacter() to query what the user typed last.
  ezUInt32 m_LastCharacter; // [tested]

private:

  /// \brief Calls InitializeDevice() when the device is not yet initialized.
  void Initialize();
  bool m_bInitialized;

  /// \brief Override this if you need to do device specific initialization before the first use.
  virtual void InitializeDevice() = 0;

  /// \brief Override this, if you need to query the state of the hardware to update the input slots.
  ///
  /// \note This function might be called multiple times before ResetInputSlotValues() is called.
  /// This will be the case when ezInputManager::PollHardware is used to make more frequent hardware updates
  /// than input is actually processed.
  /// Just make sure to always accumulate delta values (such as mouse move values) and don't expect ResetInputSlotValues()
  /// to be called in tandem with this function and it will be fine.
  virtual void UpdateInputSlotValues() = 0;

  /// \brief Override this, if you need to reset certain input slot values to zero, after the ezInputManager is finished with the current frame update.
  virtual void ResetInputSlotValues() { }; // [tested]

  /// \brief Override this to register all the input slots that this device exposes.
  ///
  /// This is called once during initialization. It needs to call RegisterInputSlot() once for every input slot that this device
  /// exposes to the system.
  virtual void RegisterInputSlots() = 0; // [tested]

  /// \brief This function is called once after ezInputManager::Update with the same time delta value.
  /// It allows to update hardware state, such as the vibration of gamepad motors.
  virtual void UpdateHardwareState(ezTime tTimeDifference) { }
};

