#include <PCH.h>
#include <GameEngine/Components/PropertyAnimComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/Curves/ColorGradientResource.h>
#include <GameEngine/Curves/Curve1DResource.h>
#include <Foundation/Reflection/ReflectionUtils.h>

EZ_BEGIN_COMPONENT_TYPE(ezPropertyAnimComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Animation", GetPropertyAnimFile, SetPropertyAnimFile)->AddAttributes(new ezAssetBrowserAttribute("PropertyAnim")),
    EZ_ENUM_MEMBER_PROPERTY("Mode", ezPropertyAnimMode, m_AnimationMode),
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
}

void ezPropertyAnimComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hPropertyAnim;

  /// \todo Somehow store the animation state (not necessary for new scenes, but for quicksaves)
}

void ezPropertyAnimComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_hPropertyAnim;
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
    // Game objects only support to animate Position, Rotation (TBD),
    // Non-Uniform Scale and the one single-float Uniform scale value
    if (pPropRtti != ezGetStaticRTTI<float>())
      return;
  }
  else
  {
    if (pPropRtti != ezGetStaticRTTI<ezVec2>() &&
      pPropRtti != ezGetStaticRTTI<ezVec3>() &&
      pPropRtti != ezGetStaticRTTI<ezVec4>())
      return;
  }

  GameObjectBinding& binding = m_GoFloatBindings.ExpandAndGetRef();
  binding.m_hObject = hGameObject;
  binding.m_pObject = pObject;
  binding.m_pAnimation = pAnim; // we can store a direct pointer here, because our sharedptr keeps the descriptor alive
  binding.m_pMemberProperty = pMember;
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
      pPropRtti != ezGetStaticRTTI<ezInt32>() &&
      pPropRtti != ezGetStaticRTTI<ezInt16>() &&
      pPropRtti != ezGetStaticRTTI<ezInt8>() &&
      pPropRtti != ezGetStaticRTTI<ezUInt32>() &&
      pPropRtti != ezGetStaticRTTI<ezUInt16>() &&
      pPropRtti != ezGetStaticRTTI<ezUInt8>())
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
    return;

  ComponentFloatBinding& binding = m_ComponentFloatBindings.ExpandAndGetRef();
  binding.m_hComponent = hComponent;
  binding.m_pObject = pObject;
  binding.m_pAnimation = pAnim; // we can store a direct pointer here, because our sharedptr keeps the descriptor alive
  binding.m_pMemberProperty = pMember;
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
  const ezTime duration = m_AnimDesc->m_AnimationDuration;

  if (m_AnimationMode == ezPropertyAnimMode::Once)
  {
    m_AnimationTime += tDiff;

    if (m_AnimationTime > duration)
    {
      m_AnimationTime = duration;
      SetActive(false);
    }
  }
  else if (m_AnimationMode == ezPropertyAnimMode::Loop)
  {
    m_AnimationTime += tDiff;

    while (m_AnimationTime > duration)
      m_AnimationTime -= duration;
  }
  else if (m_AnimationMode == ezPropertyAnimMode::BackAndForth)
  {
    if (m_bReverse)
      m_AnimationTime -= tDiff;
    else
      m_AnimationTime += tDiff;

    // ping pong back and forth as long as the current animation time is outside the valid range
    while (true)
    {
      if (m_AnimationTime > duration)
      {
        m_AnimationTime = duration - (m_AnimationTime - duration);
        m_bReverse = true;
      }
      else if (m_AnimationTime < ezTime::Zero())
      {
        m_AnimationTime = -m_AnimationTime;
        m_bReverse = false;
      }
      else
        break;
    }
  }

  return m_AnimationTime.GetSeconds();
}

void ezPropertyAnimComponent::ApplyFloatAnimation(const FloatBinding& binding, double lookupTime)
{
  const ezRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  // and now for the tedious task of applying different type values...
  // there might be better ways to do this, but I haven't checked

  float fFinalValue = 0;
  {
    const ezCurve1D& curve = binding.m_pAnimation->m_Curve;

    if (curve.IsEmpty())
      return;

    fFinalValue = (float)curve.Evaluate(lookupTime);
  }

  if (pRtti == ezGetStaticRTTI<ezVec3>())
  {
    ezTypedMemberProperty<ezVec3>* pTyped = static_cast<ezTypedMemberProperty<ezVec3>*>(binding.m_pMemberProperty);
    ezVec3 value = pTyped->GetValue(binding.m_pObject);

    if (binding.m_pAnimation->m_Target == ezPropertyAnimTarget::VectorX)
      value.x = fFinalValue;
    else if (binding.m_pAnimation->m_Target == ezPropertyAnimTarget::VectorY)
      value.y = fFinalValue;
    else if (binding.m_pAnimation->m_Target == ezPropertyAnimTarget::VectorZ)
      value.z = fFinalValue;

    pTyped->SetValue(binding.m_pObject, value);
    return;
  }

  if (pRtti == ezGetStaticRTTI<ezVec2>())
  {
    ezTypedMemberProperty<ezVec2>* pTyped = static_cast<ezTypedMemberProperty<ezVec2>*>(binding.m_pMemberProperty);
    ezVec2 value = pTyped->GetValue(binding.m_pObject);

    if (binding.m_pAnimation->m_Target == ezPropertyAnimTarget::VectorX)
      value.x = fFinalValue;
    else if (binding.m_pAnimation->m_Target == ezPropertyAnimTarget::VectorY)
      value.y = fFinalValue;

    pTyped->SetValue(binding.m_pObject, value);
    return;
  }

  if (pRtti == ezGetStaticRTTI<ezVec4>())
  {
    ezTypedMemberProperty<ezVec4>* pTyped = static_cast<ezTypedMemberProperty<ezVec4>*>(binding.m_pMemberProperty);
    ezVec4 value = pTyped->GetValue(binding.m_pObject);

    if (binding.m_pAnimation->m_Target == ezPropertyAnimTarget::VectorX)
      value.x = fFinalValue;
    else if (binding.m_pAnimation->m_Target == ezPropertyAnimTarget::VectorY)
      value.y = fFinalValue;
    else if (binding.m_pAnimation->m_Target == ezPropertyAnimTarget::VectorZ)
      value.z = fFinalValue;
    else if (binding.m_pAnimation->m_Target == ezPropertyAnimTarget::VectorW)
      value.w = fFinalValue;

    pTyped->SetValue(binding.m_pObject, value);
    return;
  }

  if (pRtti == ezGetStaticRTTI<bool>())
  {
    ezTypedMemberProperty<bool>* pTyped = static_cast<ezTypedMemberProperty<bool>*>(binding.m_pMemberProperty);

    pTyped->SetValue(binding.m_pObject, fFinalValue < 0.5f);
    return;
  }

  /// \todo etc. int types

  ezVariant value = fFinalValue;
  if (pRtti->GetVariantType() != ezVariantType::Invalid && value.CanConvertTo(pRtti->GetVariantType()))
  {
    ezReflectionUtils::SetMemberPropertyValue(binding.m_pMemberProperty, binding.m_pObject, value);
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


