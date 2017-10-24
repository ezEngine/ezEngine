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
  m_AnimBindings.Clear();
  m_AnimDesc = nullptr;

  if (!m_hPropertyAnim.IsValid())
    return;

  ezResourceLock<ezPropertyAnimResource> pAnimation(m_hPropertyAnim, ezResourceAcquireMode::NoFallback);

  if (!pAnimation || pAnimation->IsMissingResource())
    return;

  m_AnimDesc = pAnimation->GetDescriptor();

  const ezArrayPtr<ezComponent* const> AllComponents = GetOwner()->GetComponents();

  for (const ezPropertyAnimEntry& anim : m_AnimDesc->m_Animations)
  {
    // allow to animate properties on the ezGameObject
    CreatePropertyBinding(&anim, ezGetStaticRTTI<ezGameObject>(), GetOwner(), ezComponentHandle());

    // and of course all components attached to that object
    for (ezComponent* pComp : AllComponents)
    {
      CreatePropertyBinding(&anim, pComp->GetDynamicRTTI(), pComp, pComp->GetHandle());
    }

  }
}

void ezPropertyAnimComponent::CreatePropertyBinding(const ezPropertyAnimEntry* pAnim, const ezRTTI* pOwnerRtti, void* pObject, const ezComponentHandle& hComponent)
{
  ezAbstractProperty* pAbstract = pOwnerRtti->FindPropertyByName(pAnim->m_sPropertyName);

  if (pAbstract == nullptr)
    return;

  // we only support direct member properties at this time, so no arrays or other complex structures
  if (pAbstract->GetCategory() != ezPropertyCategory::Member)
    return;

  ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbstract);

  const ezRTTI* pPropRtti = pMember->GetSpecificType();

  Binding binding;
  binding.m_AnimationTime.SetZero();
  binding.m_hComponent = hComponent;
  binding.m_pObject = pObject;
  binding.m_pAnimation = pAnim; // we can store a direct pointer here, because our sharedptr keeps the descriptor alive
  binding.m_pMemberProperty = pMember;

  if (pAnim->m_Target == ezPropertyAnimTarget::Color)
  {
    if (pPropRtti != ezGetStaticRTTI<ezColor>() && pPropRtti != ezGetStaticRTTI<ezColorGammaUB>())
      return;

    if (!pAnim->m_hColorCurve.IsValid())
      return;

    m_AnimBindings.PushBack(binding);
  }

  // all others use this curve
  if (!pAnim->m_hNumberCurve.IsValid())
    return;

  if (pAnim->m_Target == ezPropertyAnimTarget::Number)
  {
    if (pPropRtti != ezGetStaticRTTI<float>() &&
        pPropRtti != ezGetStaticRTTI<bool>() &&
        pPropRtti != ezGetStaticRTTI<ezInt32>() &&
        pPropRtti != ezGetStaticRTTI<ezUInt32>()) /// \todo and all other int types, I guess ...
      return;

    m_AnimBindings.PushBack(binding);
  }

  if (pAnim->m_Target >= ezPropertyAnimTarget::VectorX && pAnim->m_Target <= ezPropertyAnimTarget::VectorW)
  {
    if (pPropRtti != ezGetStaticRTTI<ezVec2>() &&
        pPropRtti != ezGetStaticRTTI<ezVec3>() &&
        pPropRtti != ezGetStaticRTTI<ezVec4>()) /// \todo and int vector types ...
      return;

    m_AnimBindings.PushBack(binding);
  }

}


void ezPropertyAnimComponent::ApplyAnimations(const ezTime& tDiff)
{
  for (ezUInt32 i = 0; i < m_AnimBindings.GetCount(); )
  {
    auto& binding = m_AnimBindings[i];

    // if we have a component handle, use it to check that the component is still alive
    if (!binding.m_hComponent.IsInvalidated())
    {
      if (!GetWorld()->IsValidComponent(binding.m_hComponent))
      {
        // remove dead references
        m_AnimBindings.RemoveAtSwap(i);
        continue;
      }
    }

    ApplyAnimation(tDiff, i);

    ++i;
  }
}

float ezPropertyAnimComponent::ComputeAnimationLookup(ezTime& inout_tCur, const ezPropertyAnimEntry* pAnimation) const
{
  if (pAnimation->m_Mode == ezPropertyAnimMode::Once)
  {
    inout_tCur = ezMath::Min(inout_tCur, pAnimation->m_Duration);
  }
  else if (pAnimation->m_Mode == ezPropertyAnimMode::Loop)
  {
    while (inout_tCur > pAnimation->m_Duration)
      inout_tCur -= pAnimation->m_Duration;
  }
  else if (pAnimation->m_Mode == ezPropertyAnimMode::BackAndForth)
  {
    /// \todo Implement this animation mode
    // same as Loop for now

    while (inout_tCur > pAnimation->m_Duration)
      inout_tCur -= pAnimation->m_Duration;
  }

  const float lookup = (float)(inout_tCur.GetSeconds() / pAnimation->m_Duration.GetSeconds());
  return lookup;
}

void ezPropertyAnimComponent::ApplyAnimation(const ezTime& tDiff, ezUInt32 idx)
{
  auto& binding = m_AnimBindings[idx];

  binding.m_AnimationTime += tDiff;
  const float fLookupPos = ComputeAnimationLookup(binding.m_AnimationTime, binding.m_pAnimation);

  const ezRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  // and now for the tedious task of applying different type values...
  // there might be better ways to do this, but I haven't checked

  if (pRtti == ezGetStaticRTTI<ezColor>())
  {
    ezResourceLock<ezColorGradientResource> pResource(binding.m_pAnimation->m_hColorCurve);

    ezColorGammaUB gamma;
    float intensity;
    pResource->GetDescriptor().m_Gradient.Evaluate(fLookupPos, gamma, intensity);

    ezColor finalColor = gamma;
    finalColor.ScaleRGB(intensity);
    binding.m_pMemberProperty->SetValuePtr(binding.m_pObject, &finalColor);
    return;
  }

  if (pRtti == ezGetStaticRTTI<ezColorGammaUB>())
  {
    ezResourceLock<ezColorGradientResource> pResource(binding.m_pAnimation->m_hColorCurve);

    ezColorGammaUB gamma;
    float intensity;
    pResource->GetDescriptor().m_Gradient.Evaluate(fLookupPos, gamma, intensity);
    binding.m_pMemberProperty->SetValuePtr(binding.m_pObject, &gamma);
    return;
  }

  float fFinalValue = 0;
  {
    ezResourceLock<ezCurve1DResource> pResource(binding.m_pAnimation->m_hNumberCurve);
    if (pResource->GetDescriptor().m_Curves.IsEmpty())
      return;

    const ezCurve1D& curve = pResource->GetDescriptor().m_Curves[0];

    if (curve.IsEmpty())
      return;

    double fMin, fMax;
    curve.QueryExtents(fMin, fMax);

    fFinalValue = (float)curve.Evaluate(fMin + fLookupPos * (fMax - fMin));
  }

  /// \todo etc. int types
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

  ezVariant value = fFinalValue;
  if (pRtti == ezGetStaticRTTI<bool>())
  {
    value = fFinalValue < 0.5f;
  }
  if (pRtti->GetVariantType() != ezVariantType::Invalid && value.CanConvertTo(pRtti->GetVariantType()))
  {
    ezReflectionUtils::SetMemberPropertyValue(binding.m_pMemberProperty, binding.m_pObject, value);
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


