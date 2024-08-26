#pragma once

#include <Foundation/Math/Declarations.h>
#include <Foundation/Reflection/Reflection.h>

/// \brief Available procedural curve generators.
///
/// The easing function types are adapted from https://easings.net. Visit that page for a preview.
///
/// Types:
/// - EaseIn: Indicates a slow transition from the zero strength to full strength.
/// - EaseOut: Indicates a fast transition from the zero strength to full strength.
/// - EaseInOut: A transition from zero to one that starts out slow, continues fast in the middle and finishes slow again.
struct ezCurveFunction
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Linear,

    ConstantZero,
    ConstantOne,

    EaseInSine,
    EaseOutSine,
    EaseInOutSine,

    EaseInQuad,
    EaseOutQuad,
    EaseInOutQuad,

    EaseInCubic,
    EaseOutCubic,
    EaseInOutCubic,

    EaseInQuartic,
    EaseOutQuartic,
    EaseInOutQuartic,

    EaseInQuintic,
    EaseOutQuintic,
    EaseInOutQuintic,

    EaseInExpo,
    EaseOutExpo,
    EaseInOutExpo,

    EaseInCirc,
    EaseOutCirc,
    EaseInOutCirc,

    EaseInBack,       ///< Values exceed the 0-1 range briefly
    EaseOutBack,      ///< Values exceed the 0-1 range briefly
    EaseInOutBack,    ///< Values exceed the 0-1 range briefly

    EaseInElastic,    ///< Values exceed the 0-1 range briefly
    EaseOutElastic,   ///< Values exceed the 0-1 range briefly
    EaseInOutElastic, ///< Values exceed the 0-1 range briefly

    EaseInBounce,
    EaseOutBounce,
    EaseInOutBounce,

    Conical,
    FadeInHoldFadeOut,
    FadeInFadeOut,
    Bell,

    ENUM_COUNT, // All easing function types must be stated before this.

    Default = Linear
  };

  /// \brief Helper function that returns the function value at the given x coordinate.
  static double GetValue(Enum function, double x);

  /// \brief Helper function that returns the function value at the given x coordinate.
  ///
  /// If \a inverse is true, the value (1-Y) is returned.
  static double GetValue(Enum function, double x, bool bInverse);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezCurveFunction);

namespace ezMath
{
  double GetCurveValue_Linear(double t);
  double GetCurveValue_ConstantZero(double t);
  double GetCurveValue_ConstantOne(double t);
  double GetCurveValue_EaseInSine(double t);
  double GetCurveValue_EaseOutSine(double t);
  double GetCurveValue_EaseInOutSine(double t);
  double GetCurveValue_EaseInQuad(double t);
  double GetCurveValue_EaseOutQuad(double t);
  double GetCurveValue_EaseInOutQuad(double t);
  double GetCurveValue_EaseInCubic(double t);
  double GetCurveValue_EaseOutCubic(double t);
  double GetCurveValue_EaseInOutCubic(double t);
  double GetCurveValue_EaseInQuartic(double t);
  double GetCurveValue_EaseOutQuartic(double t);
  double GetCurveValue_EaseInOutQuartic(double t);
  double GetCurveValue_EaseInQuintic(double t);
  double GetCurveValue_EaseOutQuintic(double t);
  double GetCurveValue_EaseInOutQuintic(double t);
  double GetCurveValue_EaseInExpo(double t);
  double GetCurveValue_EaseOutExpo(double t);
  double GetCurveValue_EaseInOutExpo(double t);
  double GetCurveValue_EaseInCirc(double t);
  double GetCurveValue_EaseOutCirc(double t);
  double GetCurveValue_EaseInOutCirc(double t);
  double GetCurveValue_EaseInBack(double t);
  double GetCurveValue_EaseOutBack(double t);
  double GetCurveValue_EaseInOutBack(double t);
  double GetCurveValue_EaseInElastic(double t);
  double GetCurveValue_EaseOutElastic(double t);
  double GetCurveValue_EaseInOutElastic(double t);
  double GetCurveValue_EaseInBounce(double t);
  double GetCurveValue_EaseOutBounce(double t);
  double GetCurveValue_EaseInOutBounce(double t);
  double GetCurveValue_Conical(double t);
  double GetCurveValue_FadeInHoldFadeOut(double t);
  double GetCurveValue_FadeInFadeOut(double t);
  double GetCurveValue_Bell(double t);

} // namespace ezMath

#include <Foundation/Math/Implementation/CurveFunctions_inl.h>
