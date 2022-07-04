
#include <Foundation/Math/Easing.h>

namespace ezMath
{
  template <typename Type>
  Type EZ_FOUNDATION_DLL GetEasingValue(ezEasingFunctions easingFunction, Type input)
  {
    switch (easingFunction)
    {
      case InLinear:
        return EaseInLinear<Type>(input);
        break;
      case OutLinear:
        return EaseOutLinear<Type>(input);
        break;
      case InSine:
        return EaseInSine<Type>(input);
        break;
      case OutSine:
        return EaseOutSine<Type>(input);
        break;
      case InOutSine:
        return EaseInOutSine<Type>(input);
        break;
      case InQuad:
        return EaseInQuad<Type>(input);
        break;
      case OutQuad:
        return EaseOutQuad<Type>(input);
        break;
      case InOutQuad:
        return EaseInOutQuad<Type>(input);
        break;
      case InCubic:
        return EaseInCubic<Type>(input);
        break;
      case OutCubic:
        return EaseOutCubic<Type>(input);
        break;
      case InOutCubic:
        return EaseInOutCubic<Type>(input);
        break;
      case InQuartic:
        return EaseInQuartic<Type>(input);
        break;
      case OutQuartic:
        return EaseOutQuartic<Type>(input);
        break;
      case InOutQuartic:
        return EaseInOutQuartic<Type>(input);
        break;
      case InQuintic:
        return EaseInQuintic<Type>(input);
        break;
      case OutQuintic:
        return EaseOutQuintic<Type>(input);
        break;
      case InOutQuintic:
        return EaseInOutQuintic<Type>(input);
        break;
      case InExpo:
        return EaseInExpo<Type>(input);
        break;
      case OutExpo:
        return EaseOutExpo<Type>(input);
        break;
      case InOutExpo:
        return EaseInOutExpo<Type>(input);
        break;
      case InCirc:
        return EaseInCirc<Type>(input);
        break;
      case OutCirc:
        return EaseOutCirc<Type>(input);
        break;
      case InOutCirc:
        return EaseInOutCirc<Type>(input);
        break;
      case InBack:
        return EaseInBack<Type>(input);
        break;
      case OutBack:
        return EaseOutBack<Type>(input);
        break;
      case InOutBack:
        return EaseInOutBack<Type>(input);
        break;
      case InElastic:
        return EaseInElastic<Type>(input);
        break;
      case OutElastic:
        return EaseOutElastic<Type>(input);
        break;
      case InOutElastic:
        return EaseInOutElastic<Type>(input);
        break;
      case InBounce:
        return EaseInBounce<Type>(input);
        break;
      case OutBounce:
        return EaseOutBounce<Type>(input);
        break;
      case InOutBounce:
        return EaseInOutBounce<Type>(input);
        break;
      case EasingCount:
      default:
        EZ_ASSERT_NOT_IMPLEMENTED("Easing function not implemented");
        return 0;
    }
  }
} // namespace ezMath

EZ_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Easing);
