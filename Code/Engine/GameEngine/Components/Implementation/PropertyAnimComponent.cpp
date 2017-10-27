#include <PCH.h>
#include <GameEngine/Components/PropertyAnimComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <GameEngine/Curves/ColorGradientResource.h>
#include <GameEngine/Curves/Curve1DResource.h>
#include <Foundation/Reflection/ReflectionUtils.h>

EZ_BEGIN_COMPONENT_TYPE(ezPropertyAnimComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Animation", GetPropertyAnimFile, SetPropertyAnimFile)->AddAttributes(new ezAssetBrowserAttribute("PropertyAnim")),
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
  m_FloatBindings.Clear();
  m_AnimDesc = nullptr;

  if (!m_hPropertyAnim.IsValid())
    return;

  ezResourceLock<ezPropertyAnimResource> pAnimation(m_hPropertyAnim, ezResourceAcquireMode::NoFallback);

  if (!pAnimation || pAnimation->IsMissingResource())
    return;

  m_AnimDesc = pAnimation->GetDescriptor();

  const ezArrayPtr<ezComponent* const> AllComponents = GetOwner()->GetComponents();

  for (const ezFloatPropertyAnimEntry& anim : m_AnimDesc->m_FloatAnimations)
  {
    // allow to animate properties on the ezGameObject
    CreateFloatPropertyBinding(&anim, ezGetStaticRTTI<ezGameObject>(), GetOwner(), ezComponentHandle());

    // and of course all components attached to that object
    for (ezComponent* pComp : AllComponents)
    {
      CreateFloatPropertyBinding(&anim, pComp->GetDynamicRTTI(), pComp, pComp->GetHandle());
    }
  }

  for (const ezColorPropertyAnimEntry& anim : m_AnimDesc->m_ColorAnimations)
  {
    for (ezComponent* pComp : AllComponents)
    {
      CreateColorPropertyBinding(&anim, pComp->GetDynamicRTTI(), pComp, pComp->GetHandle());
    }
  }
}

void ezPropertyAnimComponent::CreateFloatPropertyBinding(const ezFloatPropertyAnimEntry* pAnim, const ezRTTI* pOwnerRtti, void* pObject, const ezComponentHandle& hComponent)
{
  if (pAnim->m_Target < ezPropertyAnimTarget::Number || pAnim->m_Target > ezPropertyAnimTarget::VectorW)
    return;

  ezAbstractProperty* pAbstract = pOwnerRtti->FindPropertyByName(pAnim->m_sPropertyName);

  if (pAbstract == nullptr)
    return;

  // we only support direct member properties at this time, so no arrays or other complex structures
  if (pAbstract->GetCategory() != ezPropertyCategory::Member)
    return;

  ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbstract);

  const ezRTTI* pPropRtti = pMember->GetSpecificType();

  FloatBinding binding;
  binding.m_hComponent = hComponent;
  binding.m_pObject = pObject;
  binding.m_pAnimation = pAnim; // we can store a direct pointer here, because our sharedptr keeps the descriptor alive
  binding.m_pMemberProperty = pMember;

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

    m_FloatBindings.PushBack(binding);
  }

  if (pAnim->m_Target >= ezPropertyAnimTarget::VectorX && pAnim->m_Target <= ezPropertyAnimTarget::VectorW)
  {
    if (pPropRtti != ezGetStaticRTTI<ezVec2>() &&
        pPropRtti != ezGetStaticRTTI<ezVec3>() &&
        pPropRtti != ezGetStaticRTTI<ezVec4>())
      return;

    m_FloatBindings.PushBack(binding);
  }
}


void ezPropertyAnimComponent::CreateColorPropertyBinding(const ezColorPropertyAnimEntry* pAnim, const ezRTTI* pOwnerRtti, void* pObject, const ezComponentHandle& hComponent)
{
  if (pAnim->m_Target != ezPropertyAnimTarget::Color)
    return;

  ezAbstractProperty* pAbstract = pOwnerRtti->FindPropertyByName(pAnim->m_sPropertyName);

  if (pAbstract == nullptr)
    return;

  // we only support direct member properties at this time, so no arrays or other complex structures
  if (pAbstract->GetCategory() != ezPropertyCategory::Member)
    return;

  ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbstract);

  const ezRTTI* pPropRtti = pMember->GetSpecificType();

  ColorBinding binding;
  binding.m_hComponent = hComponent;
  binding.m_pObject = pObject;
  binding.m_pAnimation = pAnim; // we can store a direct pointer here, because our sharedptr keeps the descriptor alive
  binding.m_pMemberProperty = pMember;

  {
    if (pPropRtti != ezGetStaticRTTI<ezColor>() && pPropRtti != ezGetStaticRTTI<ezColorGammaUB>())
      return;

    m_ColorBindings.PushBack(binding);
  }
}

void ezPropertyAnimComponent::ApplyAnimations(const ezTime& tDiff)
{
  m_AnimationTime += tDiff;

  const double fLookupPos = ComputeAnimationLookup(m_AnimationTime, m_AnimDesc->m_Mode, m_AnimDesc->m_AnimationDuration);

  for (ezUInt32 i = 0; i < m_FloatBindings.GetCount(); )
  {
    const auto& binding = m_FloatBindings[i];

    // if we have a component handle, use it to check that the component is still alive
    if (!binding.m_hComponent.IsInvalidated())
    {
      if (!GetWorld()->IsValidComponent(binding.m_hComponent))
      {
        // remove dead references
        m_FloatBindings.RemoveAtSwap(i);
        continue;
      }
    }

    ApplyFloatAnimation(i, fLookupPos);

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

    ApplyColorAnimation(i, fLookupPos);

    ++i;
  }
}

double ezPropertyAnimComponent::ComputeAnimationLookup(ezTime& inout_tCur, ezPropertyAnimMode::Enum mode, ezTime duration) const
{
  if (mode == ezPropertyAnimMode::Once)
  {
    inout_tCur = ezMath::Min(inout_tCur, duration);
  }
  else if (mode == ezPropertyAnimMode::Loop)
  {
    while (inout_tCur > duration)
      inout_tCur -= duration;
  }
  else if (mode == ezPropertyAnimMode::BackAndForth)
  {
    /// \todo Implement this animation mode
    // same as Loop for now

    while (inout_tCur > duration)
      inout_tCur -= duration;
  }

  return inout_tCur.GetSeconds() / duration.GetSeconds();
}

void ezPropertyAnimComponent::ApplyFloatAnimation(ezUInt32 idx, double lookupTime)
{
  const auto& binding = m_FloatBindings[idx];

  const ezRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  // and now for the tedious task of applying different type values...
  // there might be better ways to do this, but I haven't checked

  float fFinalValue = 0;
  {
    const ezCurve1D& curve = m_FloatBindings[idx].m_pAnimation->m_Curve;

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

void ezPropertyAnimComponent::ApplyColorAnimation(ezUInt32 idx, double lookupTime)
{
  const auto& binding = m_ColorBindings[idx];

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


