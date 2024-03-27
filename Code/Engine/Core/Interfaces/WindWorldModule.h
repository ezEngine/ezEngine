#pragma once

#include <Core/World/WorldModule.h>

/// \brief Defines the strength / speed of wind. Inspired by the Beaufort Scale.
///
/// See https://en.wikipedia.org/wiki/Beaufort_scale
struct EZ_CORE_DLL ezWindStrength
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Calm,
    LightBreeze,
    GentleBreeze,
    ModerateBreeze,
    StrongBreeze,
    Storm,
    WeakShockwave,
    MediumShockwave,
    StrongShockwave,
    ExtremeShockwave,

    Default = LightBreeze
  };

  /// \brief Maps the wind strength name to a meters per second speed value as defined by the Beaufort Scale.
  ///
  /// The value only defines how fast wind moves, how much it affects an object, like bending it, depends
  /// on additional factors like stiffness and is thus object specific.
  static float GetInMetersPerSecond(ezWindStrength::Enum strength);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezWindStrength);

class EZ_CORE_DLL ezWindWorldModuleInterface : public ezWorldModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezWindWorldModuleInterface, ezWorldModule);

protected:
  ezWindWorldModuleInterface(ezWorld* pWorld);

public:
  virtual ezVec3 GetWindAt(const ezVec3& vPosition) const = 0;
  virtual ezSimdVec4f GetWindAtSimd(const ezSimdVec4f& vPosition) const;

  /// \brief Computes a 'fluttering' wind motion orthogonal to an object direction.
  ///
  /// This is used to apply sideways or upwards wind forces on an object, such that it flutters in the wind,
  /// even when the wind is constant.
  ///
  /// \param vWind The sampled (and potentially boosted or clamped) wind value.
  /// \param vObjectDir The main direction of the object. For example the (average) direction of a tree branch, or the direction of a rope or cable. The flutter value will be orthogonal to the object direction and the wind direction. So when wind blows sideways onto a branch, the branch would flutter upwards and downwards. For a rope hanging downwards, wind blowing against it would make it flutter sideways.
  /// \param fFlutterSpeed How fast the object shall flutter (frequency).
  /// \param uiFlutterRandomOffset A random number that adds an offset to the flutter, such that multiple objects next to each other will flutter out of phase.
  ezVec3 ComputeWindFlutter(const ezVec3& vWind, const ezVec3& vObjectDir, float fFlutterSpeed, ezUInt32 uiFlutterRandomOffset) const;
};
