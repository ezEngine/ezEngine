#pragma once

#include <Foundation/Basics/Types/Bitflags.h>
#include <Core/Basics.h>

/// \brief This struct defines the different states a key can be in.
///        All keys always go through the states 'Pressed' and 'Released', even if they are active for only one frame.
///        A key is 'Down' when it is pressed for at least two frames. It is 'Up' when it is not pressed for at least two frames.
struct EZ_CORE_DLL ezKeyState
{
  enum Enum
  {
    Up,         ///< Key is not pressed at all.
    Released,   ///< Key has just been released this frame.
    Pressed,    ///< Key has just been pressed down this frame.
    Down        ///< Key is pressed down for longer than one frame now.
  };

  /// \brief Computes the new key state from a previous key state and whether it is currently pressed or not.
  static ezKeyState::Enum GetNewKeyState(ezKeyState::Enum PrevState, bool bKeyDown);
};

/// \brief These flags are specified when registering an input slot (by a device), to define some capabilities and restrictions of the hardware.
///
/// By default you do not need to use these flags at all. However, when presenting the user with a list of 'posssible' buttons to press to map to an
/// action, these flags can be used to filter out unwanted slots.
/// For example you can filter out mouse movements by requiring that the input slot must be pressable or may not represent any axis.
/// You an additionally also use the prefix of the input slot name, to filter out all touch input slots etc. if necessary.
struct ezInputSlotFlags
{
  typedef ezUInt16 StorageType;

  enum Enum
  {
    None                      = 0,

    ReportsRelativeValues     = EZ_BIT(0),  ///< The input slot reports delta values (e.g. a mouse move), instead of absolute values.
    ValueBinaryZeroOrOne      = EZ_BIT(1),  ///< The input slot will either be zero or one. Used for all buttons and keys.
    ValueRangeZeroToOne       = EZ_BIT(2),  ///< The input slot has analog values between zero and one. Used for analog axis like the xbox triggers or thumbsticks.
    ValueRangeZeroToInf       = EZ_BIT(3),  ///< The input slot has unbounded values larger than zero. Used for all absolute positions, such as the mouse position.
    Pressable                 = EZ_BIT(4),  ///< The slot can be pressed (e.g. a key). This is not possible for an axis, such as the mouse of an analog stick.
    Holdable                  = EZ_BIT(5),  ///< The user can hold down the key. Possible for buttons, but not for axes or for wheels such as the mouse wheel.
    HalfAxis                  = EZ_BIT(6),  ///< The input slot represents one half of the actually possible data. Used for all axes (pos / neg mouse movement, thumbsticks).
    FullAxis                  = EZ_BIT(7),  ///< The input slot represents one full axis. Mostly used for devices that report absolute values, such as the mouse position or touch input positions (values between zero and one) 
    RequiresDeadZone          = EZ_BIT(8),  ///< The input slot represents hardware that should use a dead zone, otherwise it might fire prematurely. Mostly used on thumbsticks and trigger buttons.
    ValuesAreNonContinuous    = EZ_BIT(9),  ///< The values of the slot can jump around randomly, ie. the user can input arbitrary values, like the position on a touchpad
    ActivationDependsOnOthers = EZ_BIT(10), ///< Whether this slot can be activated depends on whether certain other slots are active. This is the case for touchpoints which are numbered depending on how many other touchpoints are already active.

    // Some predefined sets of flags for the most common use cases
    IsButton                  =                         ValueBinaryZeroOrOne | Pressable | Holdable,
    IsMouseWheel              = ReportsRelativeValues | ValueRangeZeroToInf  | Pressable |            HalfAxis,
    IsAnalogTrigger           =                         ValueRangeZeroToOne  | Pressable | Holdable | FullAxis | RequiresDeadZone,
    IsMosueAxisPosition       =                         ValueRangeZeroToOne  |                        FullAxis,
    IsMosueAxisMove           = ReportsRelativeValues | ValueRangeZeroToInf  |                        HalfAxis,
    IsAnalogStick             =                         ValueRangeZeroToOne  |             Holdable | HalfAxis | RequiresDeadZone,
    IsDoubleClick             =                         ValueBinaryZeroOrOne | Pressable,
    IsTouchPosition           =                         ValueRangeZeroToOne  |                        FullAxis |                    ValuesAreNonContinuous,
    IsTouchPoint              =                         ValueBinaryZeroOrOne | Pressable | Holdable |                                                       ActivationDependsOnOthers,
    IsDPad                    =                         ValueBinaryZeroOrOne | Pressable | Holdable | HalfAxis,

    Default                   = None
  };

  struct Bits
  {
    StorageType ReportsRelativeValues     : 1;
    StorageType ValueBinaryZeroOrOne      : 1;
    StorageType ValueRangeZeroToOne       : 1;
    StorageType ValueRangeZeroToInf       : 1;
    StorageType Pressable                 : 1;
    StorageType Holdable                  : 1;
    StorageType HalfAxis                  : 1;
    StorageType FullAxis                  : 1;
    StorageType RequiresDeadZone          : 1;
  };
};

EZ_DECLARE_FLAGS_OR_OPERATOR(ezInputSlotFlags);

