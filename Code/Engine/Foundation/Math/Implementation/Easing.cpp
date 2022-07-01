
#include <Foundation/Math/Easing.h>

#include <unordered_map>

namespace ezMath
{
  ezEasingFunction GetEasingFunction(ezEasingFunctions easingFunction)
  {
    static std::unordered_map<ezEasingFunctions, ezEasingFunction> easingFunctions;

    if (easingFunctions.empty())
    {
      easingFunctions[ezEasingFunctions::InLinear] = EaseInLinear<double>;
      easingFunctions[ezEasingFunctions::OutLinear] = EaseOutLinear<double>;
      easingFunctions[ezEasingFunctions::InSine] = EaseInSine<double>;
      easingFunctions[ezEasingFunctions::OutSine] = EaseOutSine<double>;
      easingFunctions[ezEasingFunctions::InOutSine] = EaseInOutSine<double>;
      easingFunctions[ezEasingFunctions::InQuad] = EaseInQuad<double>;
      easingFunctions[ezEasingFunctions::OutQuad] = EaseOutQuad<double>;
      easingFunctions[ezEasingFunctions::InOutQuad] = EaseInOutQuad<double>;
      easingFunctions[ezEasingFunctions::InCubic] = EaseInCubic<double>;
      easingFunctions[ezEasingFunctions::OutCubic] = EaseOutCubic<double>;
      easingFunctions[ezEasingFunctions::InOutCubic] = EaseInOutCubic<double>;
      easingFunctions[ezEasingFunctions::InQuartic] = EaseInQuartic<double>;
      easingFunctions[ezEasingFunctions::OutQuartic] = EaseOutQuartic<double>;
      easingFunctions[ezEasingFunctions::InOutQuartic] = EaseInOutQuartic<double>;
      easingFunctions[ezEasingFunctions::InQuintic] = EaseInQuintic<double>;
      easingFunctions[ezEasingFunctions::OutQuintic] = EaseOutQuintic<double>;
      easingFunctions[ezEasingFunctions::InOutQuintic] = EaseInOutQuintic<double>;
      easingFunctions[ezEasingFunctions::InExpo] = EaseInExpo<double>;
      easingFunctions[ezEasingFunctions::OutExpo] = EaseOutExpo<double>;
      easingFunctions[ezEasingFunctions::InOutExpo] = EaseInOutExpo<double>;
      easingFunctions[ezEasingFunctions::InCirc] = EaseInCirc<double>;
      easingFunctions[ezEasingFunctions::OutCirc] = EaseOutCirc<double>;
      easingFunctions[ezEasingFunctions::InOutCirc] = EaseInOutCirc<double>;
      easingFunctions[ezEasingFunctions::InBack] = EaseInBack<double>;
      easingFunctions[ezEasingFunctions::OutBack] = EaseOutBack<double>;
      easingFunctions[ezEasingFunctions::InOutBack] = EaseInOutBack<double>;
      easingFunctions[ezEasingFunctions::InElastic] = EaseInElastic<double>;
      easingFunctions[ezEasingFunctions::OutElastic] = EaseOutElastic<double>;
      easingFunctions[ezEasingFunctions::InOutElastic] = EaseInOutElastic<double>;
      easingFunctions[ezEasingFunctions::InBounce] = EaseInBounce<double>;
      easingFunctions[ezEasingFunctions::OutBounce] = EaseOutBounce<double>;
      easingFunctions[ezEasingFunctions::InOutBounce] = EaseInOutBounce<double>;
    }

    auto iter = easingFunctions.find(easingFunction);
    return iter == easingFunctions.end() ? nullptr : iter->second;
  }
} // namespace ezMath

EZ_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Easing);
