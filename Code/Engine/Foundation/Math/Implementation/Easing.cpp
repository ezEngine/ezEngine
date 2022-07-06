
#include <Foundation/Math/Easing.h>

namespace ezMath
{
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
        EZ_ASSERT_NOT_IMPLEMENTED("Easing function not implemented");
        return 0;
    }
  }
} // namespace ezMath

EZ_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Easing);
