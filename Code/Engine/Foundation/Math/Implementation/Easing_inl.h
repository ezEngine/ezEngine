#pragma once

#include <Foundation/Math/Math.h>

namespace ezMath
{
  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInLinear(Type t)
  {
    return t;
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseOutLinear(Type t)
  {
    return 1.0 - t;
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInSine(Type t)
  {
    return 1.0 - cos((t * ezMath::Pi<Type>()) / 2.0);
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseOutSine(Type t)
  {
    return sin((t * ezMath::Pi<Type>()) / 2.0);
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInOutSine(Type t)
  {
    return -(cos(ezMath::Pi<Type>() * t) - 1.0) / 2.0;
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInQuad(Type t)
  {
    return t * t;
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseOutQuad(Type t)
  {
    return 1.0 - (1.0 - t) * (1.0 - t);
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInOutQuad(Type t)
  {
    return t < 0.5 ? 2.0 * t * t : 1.0 - pow(-2.0 * t + 2, 2) / 2;
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInCubic(Type t)
  {
    return t * t * t;
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseOutCubic(Type t)
  {
    return 1.0 - pow(1 - t, 3.0);
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInOutCubic(Type t)
  {
    return t < 0.5 ? 4.0 * t * t * t : 1.0 - pow(-2.0 * t + 2.0, 3.0) / 2.0;
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInQuartic(Type t)
  {
    return t * t * t * t;
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseOutQuartic(Type t)
  {
    return 1.0 - pow(1.0 - t, 4.0);
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInOutQuartic(Type t)
  {
    return t < 0.5 ? 8.0 * t * t * t * t : 1.0 - pow(-2.0 * t + 2.0, 4.0) / 2.0;
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInQuintic(Type t)
  {
    return t * t * t * t * t;
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseOutQuintic(Type t)
  {
    return 1.0 - pow(1.0 - t, 5.0);
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInOutQuintic(Type t)
  {
    return t < 0.5 ? 16.0 * t * t * t * t * t : 1.0 - pow(-2.0 * t + 2.0, 5.0) / 2.0;
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInExpo(Type t)
  {
    return t == 0 ? 0 : pow(2.0, 10.0 * t - 10.0);
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseOutExpo(Type t)
  {
    return t == 1.0 ? 1.0 : 1.0 - pow(2.0, -10.0 * t);
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInOutExpo(Type t)
  {
    if (t == 0.0)
    {
      return 0.0;
    }
    else if (t == 1.0)
    {
      return 1.0;
    }
    else
    {
      return t < 0.5 ? pow(2.0, 20.0 * t - 10.0) / 2.0
                     : (2.0 - pow(2.0, -20.0 * t + 10.0)) / 2.0;
    }
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInCirc(Type t)
  {
    return 1.0 - sqrt(1.0 - pow(t, 2));
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseOutCirc(Type t)
  {
    return sqrt(1.0 - pow(t - 1.0, 2.0));
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInOutCirc(Type t)
  {
    return t < 0.5
             ? (1.0 - sqrt(1.0 - pow(2.0 * t, 2.0))) / 2.0
             : (sqrt(1.0 - pow(-2.0 * t + 2.0, 2.0)) + 1.0) / 2.0;
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInBack(Type t)
  {
    return 2.70158 * t * t * t - 1.70158 * t * t;
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseOutBack(Type t)
  {
    return 1 + 2.70158 * pow(t - 1.0, 3.0) + 1.70158 * pow(t - 1.0, 2.0);
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInOutBack(Type t)
  {
    return t < 0.5
             ? (pow(2.0 * t, 2.0) * (((1.70158 * 1.525) + 1.0) * 2 * t - (1.70158 * 1.525))) / 2.0
             : (pow(2.0 * t - 2.0, 2.0) * (((1.70158 * 1.525) + 1.0) * (t * 2.0 - 2.0) + (1.70158 * 1.525)) + 2.0) / 2.0;
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInElastic(Type t)
  {
    if (t == 0.0)
    {
      return 0.0;
    }
    else if (t == 1.0)
    {
      return 1.0;
    }
    else
    {
      return -pow(2.0, 10.0 * t - 10.0) * sin((t * 10.0 - 10.75) * ((2.0 * ezMath::Pi<Type>()) / 3.0));
    }
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseOutElastic(Type t)
  {
    if (t == 0.0)
    {
      return 0.0;
    }
    else if (t == 1.0)
    {
      return 1.0;
    }
    else
    {
      return pow(2.0, -10.0 * t) * sin((t * 10.0 - 0.75) * ((2.0 * ezMath::Pi<Type>()) / 3.0)) + 1.0;
    }
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInOutElastic(Type t)
  {
    if (t == 0.0)
    {
      return 0.0;
    }
    else if (t == 1.0)
    {
      return 1.0;
    }
    else
    {
      return t < 0.5
               ? -(pow(2.0, 20.0 * t - 10.0) * sin((20.0 * t - 11.125) * ((2 * ezMath::Pi<Type>()) / 4.5))) / 2.0
               : (pow(2.0, -20.0 * t + 10.0) * sin((20.0 * t - 11.125) * ((2 * ezMath::Pi<Type>()) / 4.5))) / 2.0 + 1.0;
    }
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInBounce(Type t)
  {
    return 1.0 - EaseOutBounce<Type>(1.0 - t);
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseOutBounce(Type t)
  {
    if (t < 1.0 / 2.75)
    {
      return 7.5625 * t * t;
    }
    else if (t < 2.0 / 2.75)
    {
      return 7.5625 * (t -= 1.5 / 2.75) * t + 0.75;
    }
    else if (t < 2.5 / 2.75)
    {
      return 7.5625 * (t -= 2.25 / 2.75) * t + 0.9375;
    }
    else
    {
      return 7.5625 * (t -= 2.625 / 2.75) * t + 0.984375;
    }
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type EaseInOutBounce(Type t)
  {
    return t < 0.5
             ? (1.0 - EaseOutBounce<Type>(1.0 - 2.0 * t)) / 2.0
             : (1.0 + EaseOutBounce<Type>(2.0 * t - 1.0)) / 2.0;
  }

  template <typename Type>
  Type GetEasingValue(ezEasingFunctions easingFunction, Type input)
  {
    switch (easingFunction)
    {
      case InLinear:
        return EaseInLinear<Type>(input);
      case OutLinear:
        return EaseOutLinear<Type>(input);
      case InSine:
        return EaseInSine<Type>(input);
      case OutSine:
        return EaseOutSine<Type>(input);
      case InOutSine:
        return EaseInOutSine<Type>(input);
      case InQuad:
        return EaseInQuad<Type>(input);
      case OutQuad:
        return EaseOutQuad<Type>(input);
      case InOutQuad:
        return EaseInOutQuad<Type>(input);
      case InCubic:
        return EaseInCubic<Type>(input);
      case OutCubic:
        return EaseOutCubic<Type>(input);
      case InOutCubic:
        return EaseInOutCubic<Type>(input);
      case InQuartic:
        return EaseInQuartic<Type>(input);
      case OutQuartic:
        return EaseOutQuartic<Type>(input);
      case InOutQuartic:
        return EaseInOutQuartic<Type>(input);
      case InQuintic:
        return EaseInQuintic<Type>(input);
      case OutQuintic:
        return EaseOutQuintic<Type>(input);
      case InOutQuintic:
        return EaseInOutQuintic<Type>(input);
      case InExpo:
        return EaseInExpo<Type>(input);
      case OutExpo:
        return EaseOutExpo<Type>(input);
      case InOutExpo:
        return EaseInOutExpo<Type>(input);
      case InCirc:
        return EaseInCirc<Type>(input);
      case OutCirc:
        return EaseOutCirc<Type>(input);
      case InOutCirc:
        return EaseInOutCirc<Type>(input);
      case InBack:
        return EaseInBack<Type>(input);
      case OutBack:
        return EaseOutBack<Type>(input);
      case InOutBack:
        return EaseInOutBack<Type>(input);
      case InElastic:
        return EaseInElastic<Type>(input);
      case OutElastic:
        return EaseOutElastic<Type>(input);
      case InOutElastic:
        return EaseInOutElastic<Type>(input);
      case InBounce:
        return EaseInBounce<Type>(input);
      case OutBounce:
        return EaseOutBounce<Type>(input);
      case InOutBounce:
        return EaseInOutBounce<Type>(input);
      case ENUM_COUNT:
      default:
        EZ_REPORT_FAILURE("Easing function not implemented");
        return 0;
    }
  }
} // namespace ezMath
