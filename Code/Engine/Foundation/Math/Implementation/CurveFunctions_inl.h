#pragma once

#include <Foundation/Math/Math.h>

namespace ezMath
{
  EZ_ALWAYS_INLINE double GetCurveValue_Linear(double t)
  {
    return t;
  }

  EZ_ALWAYS_INLINE double GetCurveValue_ConstantZero(double t)
  {
    EZ_IGNORE_UNUSED(t);
    return 0.0;
  }

  EZ_ALWAYS_INLINE double GetCurveValue_ConstantOne(double t)
  {
    EZ_IGNORE_UNUSED(t);
    return 1.0;
  }

  EZ_ALWAYS_INLINE double GetCurveValue_EaseInSine(double t)
  {
    return 1.0 - cos((t * ezMath::Pi<double>()) / 2.0);
  }

  EZ_ALWAYS_INLINE double GetCurveValue_EaseOutSine(double t)
  {
    return sin((t * ezMath::Pi<double>()) / 2.0);
  }

  EZ_ALWAYS_INLINE double GetCurveValue_EaseInOutSine(double t)
  {
    return -(cos(ezMath::Pi<double>() * t) - 1.0) / 2.0;
  }

  EZ_ALWAYS_INLINE double GetCurveValue_EaseInQuad(double t)
  {
    return t * t;
  }

  EZ_ALWAYS_INLINE double GetCurveValue_EaseOutQuad(double t)
  {
    return 1.0 - (1.0 - t) * (1.0 - t);
  }

  EZ_ALWAYS_INLINE double GetCurveValue_EaseInOutQuad(double t)
  {
    return t < 0.5 ? 2.0 * t * t : 1.0 - ezMath::Pow(-2.0 * t + 2, 2.0) / 2;
  }

  EZ_ALWAYS_INLINE double GetCurveValue_EaseInCubic(double t)
  {
    return t * t * t;
  }

  EZ_ALWAYS_INLINE double GetCurveValue_EaseOutCubic(double t)
  {
    return 1.0 - pow(1 - t, 3.0);
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseInOutCubic(double t)
  {
    return t < 0.5 ? 4.0 * t * t * t : 1.0 - ezMath::Pow(-2.0 * t + 2.0, 3.0) / 2.0;
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseInQuartic(double t)
  {
    return t * t * t * t;
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseOutQuartic(double t)
  {
    return 1.0 - ezMath::Pow(1.0 - t, 4.0);
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseInOutQuartic(double t)
  {
    return t < 0.5 ? 8.0 * t * t * t * t : 1.0 - ezMath::Pow(-2.0 * t + 2.0, 4.0) / 2.0;
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseInQuintic(double t)
  {
    return t * t * t * t * t;
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseOutQuintic(double t)
  {
    return 1.0 - ezMath::Pow(1.0 - t, 5.0);
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseInOutQuintic(double t)
  {
    return t < 0.5 ? 16.0 * t * t * t * t * t : 1.0 - ezMath::Pow(-2.0 * t + 2.0, 5.0) / 2.0;
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseInExpo(double t)
  {
    return t == 0 ? 0 : ezMath::Pow(2.0, 10.0 * t - 10.0);
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseOutExpo(double t)
  {
    return t == 1.0 ? 1.0 : 1.0 - ezMath::Pow(2.0, -10.0 * t);
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseInOutExpo(double t)
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
      return t < 0.5 ? ezMath::Pow(2.0, 20.0 * t - 10.0) / 2.0
                     : (2.0 - ezMath::Pow(2.0, -20.0 * t + 10.0)) / 2.0;
    }
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseInCirc(double t)
  {
    return 1.0 - ezMath::Sqrt(1.0 - ezMath::Pow(t, 2.0));
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseOutCirc(double t)
  {
    return ezMath::Sqrt(1.0 - ezMath::Pow(t - 1.0, 2.0));
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseInOutCirc(double t)
  {
    return t < 0.5
             ? (1.0 - ezMath::Sqrt(1.0 - ezMath::Pow(2.0 * t, 2.0))) / 2.0
             : (ezMath::Sqrt(1.0 - ezMath::Pow(-2.0 * t + 2.0, 2.0)) + 1.0) / 2.0;
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseInBack(double t)
  {
    return 2.70158 * t * t * t - 1.70158 * t * t;
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseOutBack(double t)
  {
    return 1 + 2.70158 * ezMath::Pow(t - 1.0, 3.0) + 1.70158 * ezMath::Pow(t - 1.0, 2.0);
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseInOutBack(double t)
  {
    return t < 0.5
             ? (ezMath::Pow(2.0 * t, 2.0) * (((1.70158 * 1.525) + 1.0) * 2 * t - (1.70158 * 1.525))) / 2.0
             : (ezMath::Pow(2.0 * t - 2.0, 2.0) * (((1.70158 * 1.525) + 1.0) * (t * 2.0 - 2.0) + (1.70158 * 1.525)) + 2.0) / 2.0;
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseInElastic(double t)
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
      return -ezMath::Pow(2.0, 10.0 * t - 10.0) * sin((t * 10.0 - 10.75) * ((2.0 * ezMath::Pi<double>()) / 3.0));
    }
  }


  EZ_ALWAYS_INLINE double GetCurveValue_EaseOutElastic(double t)
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
      return ezMath::Pow(2.0, -10.0 * t) * sin((t * 10.0 - 0.75) * ((2.0 * ezMath::Pi<double>()) / 3.0)) + 1.0;
    }
  }

  EZ_ALWAYS_INLINE double GetCurveValue_EaseInOutElastic(double t)
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
               ? -(ezMath::Pow(2.0, 20.0 * t - 10.0) * sin((20.0 * t - 11.125) * ((2 * ezMath::Pi<double>()) / 4.5))) / 2.0
               : (ezMath::Pow(2.0, -20.0 * t + 10.0) * sin((20.0 * t - 11.125) * ((2 * ezMath::Pi<double>()) / 4.5))) / 2.0 + 1.0;
    }
  }

  EZ_ALWAYS_INLINE double GetCurveValue_EaseInBounce(double t)
  {
    return 1.0 - GetCurveValue_EaseOutBounce(1.0 - t);
  }

  EZ_ALWAYS_INLINE double GetCurveValue_EaseOutBounce(double t)
  {
    if (t < 1.0 / 2.75)
    {
      return 7.5625 * t * t;
    }
    else if (t < 2.0 / 2.75)
    {
      t -= 1.5 / 2.75;
      return 7.5625 * t * t + 0.75;
    }
    else if (t < 2.5 / 2.75)
    {
      t -= 2.25 / 2.75;
      return 7.5625 * t * t + 0.9375;
    }
    else
    {
      t -= 2.625 / 2.75;
      return 7.5625 * t * t + 0.984375;
    }
  }

  EZ_ALWAYS_INLINE double GetCurveValue_EaseInOutBounce(double t)
  {
    return t < 0.5
             ? (1.0 - GetCurveValue_EaseOutBounce(1.0 - 2.0 * t)) / 2.0
             : (1.0 + GetCurveValue_EaseOutBounce(2.0 * t - 1.0)) / 2.0;
  }

  EZ_ALWAYS_INLINE double GetCurveValue_Conical(double t)
  {
    if (t < 0.2)
    {
      return 1.0f - ezMath::Pow(1.0 - (t * 5.0), 4.0);
    }
    else
    {
      t = (t - 0.2) / 0.8; // normalize to 0-1 range

      return 1.0 - ezMath::Pow(t, 2.0);
    }
  }

  EZ_ALWAYS_INLINE double GetCurveValue_FadeInHoldFadeOut(double t)
  {
    if (t < 0.2)
    {
      return 1.0f - ezMath::Pow(1.0 - (t * 5.0), 3.0);
    }
    else if (t > 0.8)
    {
      return 1.0 - ezMath::Pow((t - 0.8) * 5.0, 3.0);
    }
    else
    {
      return 1.0;
    }
  }

  EZ_ALWAYS_INLINE double GetCurveValue_FadeInFadeOut(double t)
  {
    if (t < 0.5)
    {
      return 1.0f - ezMath::Pow(1.0 - (t * 2.0), 3.0);
    }
    else
    {
      return 1.0 - ezMath::Pow((t - 0.5) * 2.0, 3.0);
    }
  }

  EZ_ALWAYS_INLINE double GetCurveValue_Bell(double t)
  {
    if (t < 0.25)
    {
      return (ezMath::Pow((t * 4.0), 3.0)) * 0.5;
    }
    else if (t < 0.5)
    {
      return (1.0f - ezMath::Pow(1.0 - ((t - 0.25) * 4.0), 3.0)) * 0.5 + 0.5;
    }
    else if (t < 0.75)
    {
      return (1.0f - ezMath::Pow(((t - 0.5) * 4.0), 3.0)) * 0.5 + 0.5;
    }
    else
    {
      return (ezMath::Pow(1.0 - ((t - 0.75) * 4.0), 3.0)) * 0.5;
    }
  }
} // namespace ezMath

// static
inline double ezCurveFunction::GetValue(Enum function, double x)
{
  switch (function)
  {
    case Linear:
      return ezMath::GetCurveValue_Linear(x);
    case ConstantZero:
      return ezMath::GetCurveValue_ConstantZero(x);
    case ConstantOne:
      return ezMath::GetCurveValue_ConstantOne(x);
    case EaseInSine:
      return ezMath::GetCurveValue_EaseInSine(x);
    case EaseOutSine:
      return ezMath::GetCurveValue_EaseOutSine(x);
    case EaseInOutSine:
      return ezMath::GetCurveValue_EaseInOutSine(x);
    case EaseInQuad:
      return ezMath::GetCurveValue_EaseInQuad(x);
    case EaseOutQuad:
      return ezMath::GetCurveValue_EaseOutQuad(x);
    case EaseInOutQuad:
      return ezMath::GetCurveValue_EaseInOutQuad(x);
    case EaseInCubic:
      return ezMath::GetCurveValue_EaseInCubic(x);
    case EaseOutCubic:
      return ezMath::GetCurveValue_EaseOutCubic(x);
    case EaseInOutCubic:
      return ezMath::GetCurveValue_EaseInOutCubic(x);
    case EaseInQuartic:
      return ezMath::GetCurveValue_EaseInQuartic(x);
    case EaseOutQuartic:
      return ezMath::GetCurveValue_EaseOutQuartic(x);
    case EaseInOutQuartic:
      return ezMath::GetCurveValue_EaseInOutQuartic(x);
    case EaseInQuintic:
      return ezMath::GetCurveValue_EaseInQuintic(x);
    case EaseOutQuintic:
      return ezMath::GetCurveValue_EaseOutQuintic(x);
    case EaseInOutQuintic:
      return ezMath::GetCurveValue_EaseInOutQuintic(x);
    case EaseInExpo:
      return ezMath::GetCurveValue_EaseInExpo(x);
    case EaseOutExpo:
      return ezMath::GetCurveValue_EaseOutExpo(x);
    case EaseInOutExpo:
      return ezMath::GetCurveValue_EaseInOutExpo(x);
    case EaseInCirc:
      return ezMath::GetCurveValue_EaseInCirc(x);
    case EaseOutCirc:
      return ezMath::GetCurveValue_EaseOutCirc(x);
    case EaseInOutCirc:
      return ezMath::GetCurveValue_EaseInOutCirc(x);
    case EaseInBack:
      return ezMath::GetCurveValue_EaseInBack(x);
    case EaseOutBack:
      return ezMath::GetCurveValue_EaseOutBack(x);
    case EaseInOutBack:
      return ezMath::GetCurveValue_EaseInOutBack(x);
    case EaseInElastic:
      return ezMath::GetCurveValue_EaseInElastic(x);
    case EaseOutElastic:
      return ezMath::GetCurveValue_EaseOutElastic(x);
    case EaseInOutElastic:
      return ezMath::GetCurveValue_EaseInOutElastic(x);
    case EaseInBounce:
      return ezMath::GetCurveValue_EaseInBounce(x);
    case EaseOutBounce:
      return ezMath::GetCurveValue_EaseOutBounce(x);
    case EaseInOutBounce:
      return ezMath::GetCurveValue_EaseInOutBounce(x);
    case Conical:
      return ezMath::GetCurveValue_Conical(x);
    case FadeInHoldFadeOut:
      return ezMath::GetCurveValue_FadeInHoldFadeOut(x);
    case FadeInFadeOut:
      return ezMath::GetCurveValue_FadeInFadeOut(x);
    case Bell:
      return ezMath::GetCurveValue_Bell(x);

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0.0;
}

// static
inline double ezCurveFunction::GetValue(Enum function, double x, bool bInverse)
{
  double value = GetValue(function, x);

  return bInverse ? (1.0 - value) : value;
}
