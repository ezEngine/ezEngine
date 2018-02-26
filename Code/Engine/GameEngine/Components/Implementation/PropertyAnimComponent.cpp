#include <PCH.h>
#include <GameEngine/Components/PropertyAnimComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/Curves/ColorGradientResource.h>
#include <GameEngine/Curves/Curve1DResource.h>
#include <Foundation/Reflection/ReflectionUtils.h>

EZ_BEGIN_COMPONENT_TYPE(ezPropertyAnimComponent, 2, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Animation", GetPropertyAnimFile, SetPropertyAnimFile)->AddAttributes(new ezAssetBrowserAttribute("PropertyAnim")),
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezPropertyAnimMode, m_AnimationMode),
    EZ_MEMBER_PROPERTY("RandomOffset", m_RandomOffset)->AddAttributes(new ezClampValueAttribute(ezTime::Seconds(0), ezVariant())),
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(-10.0f, +10.0f)),
    EZ_MEMBER_PROPERTY("RangeLow", m_AnimationRangeLow)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant())),
    EZ_MEMBER_PROPERTY("RangeHigh", m_AnimationRangeHigh)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(ezTime::Seconds(60 * 60))),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPropertyAnimComponent::ezPropertyAnimComponent()
{
  m_AnimationRangeHigh = ezTime::Seconds(60.0 * 60.0);
}

void ezPropertyAnimComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hPropertyAnim;
  s << m_AnimationMode;
  s << m_RandomOffset;
  s << m_fSpeed;
  s << m_AnimationTime;
  s << m_bReverse;
  s << m_AnimationRangeLow;
  s << m_AnimationRangeHigh;

  /// \todo Somehow store the animation state (not necessary for new scenes, but for quicksaves)
}

void ezPropertyAnimComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hPropertyAnim;

  if (uiVersion >= 2)
  {
    s >> m_AnimationMode;
    s >> m_RandomOffset;
    s >> m_fSpeed;
    s >> m_AnimationTime;
    s >> m_bReverse;
    s >> m_AnimationRangeLow;
    s >> m_AnimationRangeHigh;

  }
}

void ezPropertyAnimComponent::SetPropertyAnimFile(const char* szFile)
{
  ezPropertyAnimResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezPropertyAnimResource>(szFile);
  }

  SetPropertyAnim(hResource);
}

const char* ezPropertyAnimComponent::GetPropertyAnimFile() const
{
  if (!m_hPropertyAnim.IsValid())
    return "";

  return m_hPropertyAnim.GetResourceID();
}

void ezPropertyAnimComponent::SetPropertyAnim(const ezPropertyAnimResourceHandle& hPropertyAnim)
{
  m_hPropertyAnim = hPropertyAnim;
}

void ezPropertyAnimComponent::CreatePropertyBindings()
{
  m_ColorBindings.Clear();
  m_ComponentFloatBindings.Clear();
  m_GoFloatBindings.Clear();

  m_AnimDesc = nullptr;

  if (!m_hPropertyAnim.IsValid())
    return;

  ezResourceLock<ezPropertyAnimResource> pAnimation(m_hPropertyAnim, ezResourceAcquireMode::NoFallback);

  if (!pAnimation || pAnimation->IsMissingResource())
    return;

  m_AnimDesc = pAnimation->GetDescriptor();

  for (const ezFloatPropertyAnimEntry& anim : m_AnimDesc->m_FloatAnimations)
  {
    ezGameObject* pTargetObject = GetOwner()->SearchForChildByNameSequence(anim.m_sObjectSearchSequence, anim.m_pComponentRtti);
    if (pTargetObject == nullptr)
      continue;

    // allow to animate properties on the ezGameObject
    if (anim.m_pComponentRtti == nullptr)
    {
      CreateGameObjectBinding(&anim, ezGetStaticRTTI<ezGameObject>(), pTargetObject, pTargetObject->GetHandle());
    }
    else
    {
      ezComponent* pComp;
      if (pTargetObject->TryGetComponentOfBaseType(anim.m_pComponentRtti, pComp))
      {
        CreateFloatPropertyBinding(&anim, pComp->GetDynamicRTTI(), pComp, pComp->GetHandle());
      }
    }
  }

  for (const ezColorPropertyAnimEntry& anim : m_AnimDesc->m_ColorAnimations)
  {
    ezGameObject* pTargetObject = GetOwner()->SearchForChildByNameSequence(anim.m_sObjectSearchSequence, anim.m_pComponentRtti);
    if (pTargetObject == nullptr)
      continue;

    ezComponent* pComp;
    if (pTargetObject->TryGetComponentOfBaseType(anim.m_pComponentRtti, pComp))
    {
      CreateColorPropertyBinding(&anim, pComp->GetDynamicRTTI(), pComp, pComp->GetHandle());
    }
  }
}

void ezPropertyAnimComponent::CreateGameObjectBinding(const ezFloatPropertyAnimEntry* pAnim, const ezRTTI* pOwnerRtti, void* pObject, const ezGameObjectHandle& hGameObject)
{
  if (pAnim->m_Target < ezPropertyAnimTarget::Number || pAnim->m_Target > ezPropertyAnimTarget::RotationZ)
    return;

  ezAbstractProperty* pAbstract = pOwnerRtti->FindPropertyByName(pAnim->m_sPropertyPath);

  // we only support direct member properties at this time, so no arrays or other complex structures
  if (pAbstract == nullptr || pAbstract->GetCategory() != ezPropertyCategory::Member)
    return;

  ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbstract);

  const ezRTTI* pPropRtti = pMember->GetSpecificType();

  if (pAnim->m_Target == ezPropertyAnimTarget::Number)
  {
    // Game objects only support to animate Position, Rotation,
    // Non-Uniform Scale and the one single-float Uniform scale value
    if (pPropRtti != ezGetStaticRTTI<float>())
      return;
  }
  else if (pAnim->m_Target >= ezPropertyAnimTarget::RotationX && pAnim->m_Target <= ezPropertyAnimTarget::RotationZ)
  {
    if (pPropRtti != ezGetStaticRTTI<ezQuat>())
      return;
  }
  else
  {
    if (pPropRtti != ezGetStaticRTTI<ezVec2>() &&
      pPropRtti != ezGetStaticRTTI<ezVec3>() &&
      pPropRtti != ezGetStaticRTTI<ezVec4>())
      return;
  }

  GameObjectBinding* binding = nullptr;
  for (ezUInt32 i = 0; i < m_GoFloatBindings.GetCount(); ++i)
  {
    auto& b = m_GoFloatBindings[i];

    if (b.m_hObject == hGameObject && b.m_pMemberProperty == pMember && b.m_pObject == pObject)
    {
      binding = &b;
      break;
    }
  }

  if (binding == nullptr)
  {
    binding = &m_GoFloatBindings.ExpandAndGetRef();
  }

  binding->m_hObject = hGameObject;
  binding->m_pObject = pObject;
  binding->m_pMemberProperty = pMember;

  // we can store a direct pointer here, because our sharedptr keeps the descriptor alive

  if (pAnim->m_Target >= ezPropertyAnimTarget::VectorX && pAnim->m_Target <= ezPropertyAnimTarget::VectorW)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)ezPropertyAnimTarget::VectorX] = pAnim;
  }
  else if (pAnim->m_Target >= ezPropertyAnimTarget::RotationX && pAnim->m_Target <= ezPropertyAnimTarget::RotationZ)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)ezPropertyAnimTarget::RotationX] = pAnim;
  }
  else if (pAnim->m_Target >= ezPropertyAnimTarget::Number)
  {
    binding->m_pAnimation[0] = pAnim;
  }
  else
  {
    EZ_REPORT_FAILURE("Invalid animation target type '{0}'", pAnim->m_Target.GetValue());
  }
}

void ezPropertyAnimComponent::CreateFloatPropertyBinding(const ezFloatPropertyAnimEntry* pAnim, const ezRTTI* pOwnerRtti, void* pObject, const ezComponentHandle& hComponent)
{
  if (pAnim->m_Target < ezPropertyAnimTarget::Number || pAnim->m_Target > ezPropertyAnimTarget::VectorW)
    return;

  ezAbstractProperty* pAbstract = pOwnerRtti->FindPropertyByName(pAnim->m_sPropertyPath);

  // we only support direct member properties at this time, so no arrays or other complex structures
  if (pAbstract == nullptr || pAbstract->GetCategory() != ezPropertyCategory::Member)
    return;

  ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbstract);

  const ezRTTI* pPropRtti = pMember->GetSpecificType();

  if (pAnim->m_Target == ezPropertyAnimTarget::Number)
  {
    if (pPropRtti != ezGetStaticRTTI<float>() &&
      pPropRtti != ezGetStaticRTTI<double>() &&
      pPropRtti != ezGetStaticRTTI<bool>() &&
      pPropRtti != ezGetStaticRTTI<ezInt64>() &&
      pPropRtti != ezGetStaticRTTI<ezInt32>() &&
      pPropRtti != ezGetStaticRTTI<ezInt16>() &&
      pPropRtti != ezGetStaticRTTI<ezInt8>() &&
      pPropRtti != ezGetStaticRTTI<ezUInt64>() &&
      pPropRtti != ezGetStaticRTTI<ezUInt32>() &&
      pPropRtti != ezGetStaticRTTI<ezUInt16>() &&
      pPropRtti != ezGetStaticRTTI<ezUInt8>() &&
      pPropRtti != ezGetStaticRTTI<ezAngle>() &&
      pPropRtti != ezGetStaticRTTI<ezTime>())
      return;
  }
  else if (pAnim->m_Target >= ezPropertyAnimTarget::VectorX && pAnim->m_Target <= ezPropertyAnimTarget::VectorW)
  {
    if (pPropRtti != ezGetStaticRTTI<ezVec2>() &&
      pPropRtti != ezGetStaticRTTI<ezVec3>() &&
      pPropRtti != ezGetStaticRTTI<ezVec4>())
      return;
  }
  else
  {
    // Quaternions are not supported for regular types
    return;
  }

  ComponentFloatBinding* binding = nullptr;
  for (ezUInt32 i = 0; i < m_ComponentFloatBindings.GetCount(); ++i)
  {
    auto& b = m_ComponentFloatBindings[i];

    if (b.m_hComponent == hComponent && b.m_pMemberProperty == pMember && b.m_pObject == pObject)
    {
      binding = &b;
      break;
    }
  }

  if (binding == nullptr)
  {
    binding = &m_ComponentFloatBindings.ExpandAndGetRef();
  }

  binding->m_hComponent = hComponent;
  binding->m_pObject = pObject;
  binding->m_pMemberProperty = pMember;

  // we can store a direct pointer here, because our sharedptr keeps the descriptor alive
  if (pAnim->m_Target >= ezPropertyAnimTarget::VectorX && pAnim->m_Target <= ezPropertyAnimTarget::VectorW)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)ezPropertyAnimTarget::VectorX] = pAnim;
  }
  else if (pAnim->m_Target >= ezPropertyAnimTarget::RotationX && pAnim->m_Target <= ezPropertyAnimTarget::RotationZ)
  {
    binding->m_pAnimation[(int)pAnim->m_Target - (int)ezPropertyAnimTarget::RotationX] = pAnim;
  }
  else if (pAnim->m_Target >= ezPropertyAnimTarget::Number)
  {
    binding->m_pAnimation[0] = pAnim;
  }
  else
  {
    EZ_REPORT_FAILURE("Invalid animation target type '{0}'", pAnim->m_Target.GetValue());
  }
}


void ezPropertyAnimComponent::CreateColorPropertyBinding(const ezColorPropertyAnimEntry* pAnim, const ezRTTI* pOwnerRtti, void* pObject, const ezComponentHandle& hComponent)
{
  if (pAnim->m_Target != ezPropertyAnimTarget::Color)
    return;

  ezAbstractProperty* pAbstract = pOwnerRtti->FindPropertyByName(pAnim->m_sPropertyPath);

  // we only support direct member properties at this time, so no arrays or other complex structures
  if (pAbstract == nullptr || pAbstract->GetCategory() != ezPropertyCategory::Member)
    return;

  ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbstract);

  const ezRTTI* pPropRtti = pMember->GetSpecificType();

  if (pPropRtti != ezGetStaticRTTI<ezColor>() && pPropRtti != ezGetStaticRTTI<ezColorGammaUB>())
    return;

  ColorBinding& binding = m_ColorBindings.ExpandAndGetRef();
  binding.m_hComponent = hComponent;
  binding.m_pObject = pObject;
  binding.m_pAnimation = pAnim; // we can store a direct pointer here, because our sharedptr keeps the descriptor alive
  binding.m_pMemberProperty = pMember;
}

void ezPropertyAnimComponent::ApplyAnimations(const ezTime& tDiff)
{
  const double fLookupPos = ComputeAnimationLookup(tDiff);

  for (ezUInt32 i = 0; i < m_ComponentFloatBindings.GetCount(); )
  {
    const auto& binding = m_ComponentFloatBindings[i];

    // if we have a component handle, use it to check that the component is still alive
    if (!binding.m_hComponent.IsInvalidated())
    {
      if (!GetWorld()->IsValidComponent(binding.m_hComponent))
      {
        // remove dead references
        m_ComponentFloatBindings.RemoveAtSwap(i);
        continue;
      }
    }

    ApplyFloatAnimation(m_ComponentFloatBindings[i], fLookupPos);

    ++i;
  }

  for (ezUInt32 i = 0; i < m_ColorBindings.GetCount(); )
  {
    const auto& binding = m_ColorBindings[i];

    // if we have a component handle, use it to check that the component is still alive
    if (!binding.m_hComponent.IsInvalidated())
    {
      if (!GetWorld()->IsValidComponent(binding.m_hComponent))
      {
        // remove dead references
        m_ColorBindings.RemoveAtSwap(i);
        continue;
      }
    }

    ApplyColorAnimation(m_ColorBindings[i], fLookupPos);

    ++i;
  }

  for (ezUInt32 i = 0; i < m_GoFloatBindings.GetCount(); )
  {
    const auto& binding = m_GoFloatBindings[i];

    // if we have a game object handle, use it to check that the component is still alive
    if (!binding.m_hObject.IsInvalidated())
    {
      if (!GetWorld()->IsValidObject(binding.m_hObject))
      {
        // remove dead references
        m_GoFloatBindings.RemoveAtSwap(i);
        continue;
      }
    }

    ApplyFloatAnimation(m_GoFloatBindings[i], fLookupPos);

    ++i;
  }
}

double ezPropertyAnimComponent::ComputeAnimationLookup(ezTime tDiff)
{
  const ezTime duration = m_AnimationRangeHigh - m_AnimationRangeLow;

  if (duration.IsZero())
    return m_AnimationRangeLow.GetSeconds();

  tDiff = m_fSpeed * tDiff;

  if (m_AnimationMode == ezPropertyAnimMode::Once)
  {
    m_AnimationTime += tDiff;

    if (m_fSpeed > 0 && m_AnimationTime >= m_AnimationRangeHigh)
    {
      m_AnimationTime = m_AnimationRangeHigh;
      SetActive(false);

      // TODO: send event
    }
    else if (m_fSpeed < 0 && m_AnimationTime <= m_AnimationRangeLow)
    {
      m_AnimationTime = m_AnimationRangeLow;
      SetActive(false);

      // TODO: send event
    }
  }
  else if (m_AnimationMode == ezPropertyAnimMode::Loop)
  {
    m_AnimationTime += tDiff;

    while (m_AnimationTime > m_AnimationRangeHigh)
    {
      m_AnimationTime -= duration;
      // TODO: send event
    }

    while (m_AnimationTime < m_AnimationRangeLow)
    {
      m_AnimationTime += duration;
      // TODO: send event
    }
  }
  else if (m_AnimationMode == ezPropertyAnimMode::BackAndForth)
  {
    const bool bReverse = m_fSpeed < 0 ? !m_bReverse : m_bReverse;

    if (bReverse)
      m_AnimationTime -= tDiff;
    else
      m_AnimationTime += tDiff;

    // ping pong back and forth as long as the current animation time is outside the valid range
    while (true)
    {
      if (m_AnimationTime > m_AnimationRangeHigh)
      {
        m_AnimationTime = m_AnimationRangeHigh - (m_AnimationTime - m_AnimationRangeHigh);
        m_bReverse = true;

        // TODO: send event
      }
      else if (m_AnimationTime < m_AnimationRangeLow)
      {
        m_AnimationTime = m_AnimationRangeLow + (m_AnimationRangeLow - m_AnimationTime);
        m_bReverse = false;

        // TODO: send event
      }
      else
        break;
    }
  }

  return m_AnimationTime.GetSeconds();
}


void ezPropertyAnimComponent::OnSimulationStarted()
{
  CreatePropertyBindings();

  m_AnimationRangeLow = ezMath::Clamp(m_AnimationRangeLow, ezTime::Zero(), m_AnimDesc->m_AnimationDuration);
  m_AnimationRangeHigh = ezMath::Clamp(m_AnimationRangeHigh, m_AnimationRangeLow, m_AnimDesc->m_AnimationDuration);

  // when starting with a negative speed, start at the end of the animation and play backwards
  // important for play-once mode
  if (m_fSpeed < 0.0f)
  {
    m_AnimationTime = m_AnimationRangeHigh;
  }

  if (!m_RandomOffset.IsZero() && !m_AnimDesc->m_AnimationDuration.IsZeroOrLess())
  {
    // should the random offset also be scaled by the speed factor? I guess not
    m_AnimationTime += ezMath::Abs(m_fSpeed) * ezTime::Seconds(GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, m_RandomOffset.GetSeconds()));

    const ezTime duration = m_AnimationRangeHigh - m_AnimationRangeLow;

    if (duration.IsZeroOrLess())
    {
      m_AnimationTime = m_AnimationRangeLow;
    }
    else
    {
      // adjust current time to be inside the valid range
      // do not clamp, as that would give a skewed random chance
      while (m_AnimationTime > m_AnimationRangeHigh)
      {
        m_AnimationTime -= duration;
      }

      while (m_AnimationTime < m_AnimationRangeLow)
      {
        m_AnimationTime += duration;
      }
    }
  }
}

void ezPropertyAnimComponent::ApplySingleFloatAnimation(const FloatBinding& binding, double lookupTime)
{
  const ezRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  double fFinalValue = 0;
  {
    const ezCurve1D& curve = binding.m_pAnimation[0]->m_Curve;

    if (curve.IsEmpty())
      return;

    fFinalValue = curve.Evaluate(lookupTime);
  }

  if (pRtti == ezGetStaticRTTI<bool>())
  {
    ezTypedMemberProperty<bool>* pTyped = static_cast<ezTypedMemberProperty<bool>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, fFinalValue < 0.5);
    return;
  }
  else if (pRtti == ezGetStaticRTTI<ezAngle>())
  {
    ezTypedMemberProperty<ezAngle>* pTyped = static_cast<ezTypedMemberProperty<ezAngle>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, ezAngle::Degree((float)fFinalValue));
    return;
  }
  else if (pRtti == ezGetStaticRTTI<ezTime>())
  {
    ezTypedMemberProperty<ezTime>* pTyped = static_cast<ezTypedMemberProperty<ezTime>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, ezTime::Seconds(fFinalValue));
    return;
  }

  // this handles float, double, all int types, etc.
  ezVariant value = fFinalValue;
  if (pRtti->GetVariantType() != ezVariantType::Invalid && value.CanConvertTo(pRtti->GetVariantType()))
  {
    ezReflectionUtils::SetMemberPropertyValue(binding.m_pMemberProperty, binding.m_pObject, value);
  }
}

void ezPropertyAnimComponent::ApplyFloatAnimation(const FloatBinding& binding, double lookupTime)
{
  if (binding.m_pAnimation[0] != nullptr && binding.m_pAnimation[0]->m_Target == ezPropertyAnimTarget::Number)
  {
    ApplySingleFloatAnimation(binding, lookupTime);
    return;
  }

  const ezRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  float fCurValue[4] = { 0, 0, 0, 0 };

  if (pRtti == ezGetStaticRTTI<ezVec2>())
  {
    ezTypedMemberProperty<ezVec2>* pTyped = static_cast<ezTypedMemberProperty<ezVec2>*>(binding.m_pMemberProperty);
    const ezVec2 value = pTyped->GetValue(binding.m_pObject);

    fCurValue[0] = value.x;
    fCurValue[1] = value.y;
  }
  else if (pRtti == ezGetStaticRTTI<ezVec3>())
  {
    ezTypedMemberProperty<ezVec3>* pTyped = static_cast<ezTypedMemberProperty<ezVec3>*>(binding.m_pMemberProperty);
    const ezVec3 value = pTyped->GetValue(binding.m_pObject);

    fCurValue[0] = value.x;
    fCurValue[1] = value.y;
    fCurValue[2] = value.z;
  }
  else if (pRtti == ezGetStaticRTTI<ezVec4>())
  {
    ezTypedMemberProperty<ezVec4>* pTyped = static_cast<ezTypedMemberProperty<ezVec4>*>(binding.m_pMemberProperty);
    const ezVec4 value = pTyped->GetValue(binding.m_pObject);

    fCurValue[0] = value.x;
    fCurValue[1] = value.y;
    fCurValue[2] = value.z;
    fCurValue[3] = value.w;
  }
  else if (pRtti == ezGetStaticRTTI<ezQuat>())
  {
    ezTypedMemberProperty<ezQuat>* pTyped = static_cast<ezTypedMemberProperty<ezQuat>*>(binding.m_pMemberProperty);
    const ezQuat value = pTyped->GetValue(binding.m_pObject);

    ezAngle euler[3];
    value.GetAsEulerAngles(euler[0], euler[1], euler[2]);
    fCurValue[0] = euler[0].GetDegree();
    fCurValue[1] = euler[1].GetDegree();
    fCurValue[2] = euler[2].GetDegree();
  }

  // evaluate all available curves
  for (ezUInt32 i = 0; i < 4; ++i)
  {
    if (binding.m_pAnimation[i] != nullptr)
    {
      const ezCurve1D& curve = binding.m_pAnimation[i]->m_Curve;

      if (!curve.IsEmpty())
      {
        fCurValue[i] = (float)curve.Evaluate(lookupTime);
      }
    }
  }

  if (pRtti == ezGetStaticRTTI<ezVec2>())
  {
    ezTypedMemberProperty<ezVec2>* pTyped = static_cast<ezTypedMemberProperty<ezVec2>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, ezVec2(fCurValue[0], fCurValue[1]));
  }
  else if (pRtti == ezGetStaticRTTI<ezVec3>())
  {
    ezTypedMemberProperty<ezVec3>* pTyped = static_cast<ezTypedMemberProperty<ezVec3>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, ezVec3(fCurValue[0], fCurValue[1], fCurValue[2]));
  }
  else if (pRtti == ezGetStaticRTTI<ezVec4>())
  {
    ezTypedMemberProperty<ezVec4>* pTyped = static_cast<ezTypedMemberProperty<ezVec4>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, ezVec4(fCurValue[0], fCurValue[1], fCurValue[2], fCurValue[3]));
  }
  else if (pRtti == ezGetStaticRTTI<ezQuat>())
  {
    ezTypedMemberProperty<ezQuat>* pTyped = static_cast<ezTypedMemberProperty<ezQuat>*>(binding.m_pMemberProperty);

    ezQuat rot;
    rot.SetFromEulerAngles(ezAngle::Degree(fCurValue[0]), ezAngle::Degree(fCurValue[1]), ezAngle::Degree(fCurValue[2]));

    pTyped->SetValue(binding.m_pObject, rot);
  }
}

void ezPropertyAnimComponent::ApplyColorAnimation(const ColorBinding& binding, double lookupTime)
{
  const ezRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  if (pRtti == ezGetStaticRTTI<ezColorGammaUB>())
  {
    ezColorGammaUB gamma;
    float intensity;
    binding.m_pAnimation->m_Gradient.Evaluate((float)lookupTime, gamma, intensity);
    binding.m_pMemberProperty->SetValuePtr(binding.m_pObject, &gamma);
    return;
  }

  if (pRtti == ezGetStaticRTTI<ezColor>())
  {
    ezColorGammaUB gamma;
    float intensity;
    binding.m_pAnimation->m_Gradient.Evaluate((float)lookupTime, gamma, intensity);

    ezColor finalColor = gamma;
    finalColor.ScaleRGB(intensity);
    binding.m_pMemberProperty->SetValuePtr(binding.m_pObject, &finalColor);
    return;
  }
}

void ezPropertyAnimComponent::Update()
{
  if (!m_hPropertyAnim.IsValid())
    return;

  if (m_AnimDesc == nullptr)
  {
    CreatePropertyBindings();
  }

  ApplyAnimations(GetWorld()->GetClock().GetTimeDiff());
}


