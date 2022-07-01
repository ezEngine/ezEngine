#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Declarations.h>

namespace ezMath
{
  enum ezEasingFunctions
  {
    InLinear,
    OutLinear,
    InSine,
    OutSine,
    InOutSine,
    InQuad,
    OutQuad,
    InOutQuad,
    InCubic,
    OutCubic,
    InOutCubic,
    InQuartic,
    OutQuartic,
    InOutQuartic,
    InQuintic,
    OutQuintic,
    InOutQuintic,
    InExpo,
    OutExpo,
    InOutExpo,
    InCirc,
    OutCirc,
    InOutCirc,
    InBack,
    OutBack,
    InOutBack,
    InElastic,
    OutElastic,
    InOutElastic,
    InBounce,
    OutBounce,
    InOutBounce
  };

  typedef double (*ezEasingFunction)(double);

  /// \brief Helper function that returns an easing function pointer.
  ezEasingFunction EZ_FOUNDATION_DLL GetEasingFunction(ezEasingFunctions easingFunction);

  template <typename Type>
  Type EaseInLinear(Type t);

  template <typename Type>
  Type EaseOutLinear(Type t);

  template <typename Type>
  Type EaseInSine(Type t);

  template <typename Type>
  Type EaseOutSine(Type t);

  template <typename Type>
  Type EaseInOutSine(Type t);

  template <typename Type>
  Type EaseInQuad(Type t);

  template <typename Type>
  Type EaseOutQuad(Type t);

  template <typename Type>
  Type EaseInOutQuad(Type t);

  template <typename Type>
  Type EaseInCubic(Type t);

  template <typename Type>
  Type EaseOutCubic(Type t);

  template <typename Type>
  Type EaseInOutCubic(Type t);

  template <typename Type>
  Type EaseInQuartic(Type t);

  template <typename Type>
  Type EaseOutQuartic(Type t);

  template <typename Type>
  Type EaseInOutQuartic(Type t);

  template <typename Type>
  Type EaseInQuintic(Type t);

  template <typename Type>
  Type EaseOutQuintic(Type t);

  template <typename Type>
  Type EaseInOutQuintic(Type t);

  template <typename Type>
  Type EaseInExpo(Type t);

  template <typename Type>
  Type EaseOutExpo(Type t);

  template <typename Type>
  Type EaseInOutExpo(Type t);

  template <typename Type>
  Type EaseInCirc(Type t);

  template <typename Type>
  Type EaseOutCirc(Type t);

  template <typename Type>
  Type EaseInOutCirc(Type t);

  template <typename Type>
  Type EaseInBack(Type t);

  template <typename Type>
  Type EaseOutBack(Type t);

  template <typename Type>
  Type EaseInOutBack(Type t);

  template <typename Type>
  Type EaseInElastic(Type t);

  template <typename Type>
  Type EaseOutElastic(Type t);

  template <typename Type>
  Type EaseInOutElastic(Type t);

  template <typename Type>
  Type EaseInBounce(Type t);

  template <typename Type>
  Type EaseOutBounce(Type t);

  template <typename Type>
  Type EaseInOutBounce(Type t);

} // namespace ezMath

#include <Foundation/Math/Implementation/Easing_inl.h>
