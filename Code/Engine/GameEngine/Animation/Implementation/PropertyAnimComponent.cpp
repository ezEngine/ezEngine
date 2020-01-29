#include <GameEnginePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GameEngine/Animation/PropertyAnimComponent.h>
#include <GameEngine/Curves/ColorGradientResource.h>
#include <GameEngine/Curves/Curve1DResource.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_COMPONENT_TYPE(ezPropertyAnimComponent, 3, ezComponentMode::Dynamic)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_ACCESSOR_PROPERTY("Animation", GetPropertyAnimFile, SetPropertyAnimFile)->AddAttributes(new ezAssetBrowserAttribute("PropertyAnim")),
      EZ_MEMBER_PROPERTY("Playing", m_bPlaying)->AddAttributes(new ezDefaultValueAttribute(true)),
      EZ_ENUM_MEMBER_PROPERTY("Mode", ezPropertyAnimMode, m_AnimationMode),
      EZ_MEMBER_PROPERTY("RandomOffset", m_RandomOffset)->AddAttributes(new ezClampValueAttribute(ezTime::Seconds(0), ezVariant())),
      EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(-10.0f, +10.0f)),
      EZ_MEMBER_PROPERTY("RangeLow", m_AnimationRangeLow)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant())),
      EZ_MEMBER_PROPERTY("RangeHigh", m_AnimationRangeHigh)->AddAttributes(new ezClampValueAttribute(ezTime(), ezVariant()), new ezDefaultValueAttribute(ezTime::Seconds(60 * 60))),
    } EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Animation"),
    } EZ_END_ATTRIBUTES;
    EZ_BEGIN_MESSAGEHANDLERS
    {
      EZ_MESSAGE_HANDLER(ezMsgSetPlaying, OnMsgSetPlaying),
    } EZ_END_MESSAGEHANDLERS;
    EZ_BEGIN_MESSAGESENDERS
    {
      EZ_MESSAGE_SENDER(m_EventTrackMsgSender),
      EZ_MESSAGE_SENDER(m_ReachedEndMsgSender),
    } EZ_END_MESSAGESENDERS;
    EZ_BEGIN_FUNCTIONS
    {
      EZ_SCRIPT_FUNCTION_PROPERTY(PlayAnimationRange, In, "RangeLow", In, "RangeHigh")} EZ_END_FUNCTIONS;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezPropertyAnimComponent::ezPropertyAnimComponent()
{
  m_AnimationRangeHigh = ezTime::Seconds(60.0 * 60.0);
}

ezPropertyAnimComponent::~ezPropertyAnimComponent() = default;

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

  s << m_bPlaying;

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

  if (uiVersion >= 3)
  {
    s >> m_bPlaying;
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

void ezPropertyAnimComponent::PlayAnimationRange(ezTime RangeLow, ezTime RangeHigh)
{
  m_AnimationRangeLow = RangeLow;
  m_AnimationRangeHigh = RangeHigh;

  m_AnimationTime = m_AnimationRangeLow;
  m_bPlaying = true;

  StartPlayback();
}


void ezPropertyAnimComponent::OnMsgSetPlaying(ezMsgSetPlaying& msg)
{
  m_bPlaying = msg.m_bPlay;
}

void ezPropertyAnimComponent::CreatePropertyBindings()
{
  m_ColorBindings.Clear();
  m_ComponentFloatBindings.Clear();
  m_GoFloatBindings.Clear();

  m_AnimDesc = nullptr;

  if (!m_hPropertyAnim.IsValid())
    return;

  ezResourceLock<ezPropertyAnimResource> pAnimation(m_hPropertyAnim, ezResourceAcquireMode::BlockTillLoaded);

  if (!pAnimation || pAnimation.GetAcquireResult() == ezResourceAcquireResult::MissingFallback)
    return;

  m_AnimDesc = pAnimation->GetDescriptor();

  for (const ezFloatPropertyAnimEntry& anim : m_AnimDesc->m_FloatAnimations)
  {
    ezHybridArray<ezGameObject*, 8> targets;
    GetOwner()->SearchForChildrenByNameSequence(anim.m_sObjectSearchSequence, anim.m_pComponentRtti, targets);

    for (ezGameObject* pTargetObject : targets)
    {
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
  }

  for (const ezColorPropertyAnimEntry& anim : m_AnimDesc->m_ColorAnimations)
  {
    ezHybridArray<ezGameObject*, 8> targets;
    GetOwner()->SearchForChildrenByNameSequence(anim.m_sObjectSearchSequence, anim.m_pComponentRtti, targets);

    for (ezGameObject* pTargetObject : targets)
    {
      ezComponent* pComp;
      if (pTargetObject->TryGetComponentOfBaseType(anim.m_pComponentRtti, pComp))
      {
        CreateColorPropertyBinding(&anim, pComp->GetDynamicRTTI(), pComp, pComp->GetHandle());
      }
    }
  }
}

void ezPropertyAnimComponent::CreateGameObjectBinding(const ezFloatPropertyAnimEntry* pAnim, const ezRTTI* pOwnerRtti, void* pObject,
  const ezGameObjectHandle& hGameObject)
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
    if (pPropRtti != ezGetStaticRTTI<ezVec2>() && pPropRtti != ezGetStaticRTTI<ezVec3>() && pPropRtti != ezGetStaticRTTI<ezVec4>())
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

void ezPropertyAnimComponent::CreateFloatPropertyBinding(const ezFloatPropertyAnimEntry* pAnim, const ezRTTI* pOwnerRtti, void* pObject,
  const ezComponentHandle& hComponent)
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
    if (pPropRtti != ezGetStaticRTTI<float>() && pPropRtti != ezGetStaticRTTI<double>() && pPropRtti != ezGetStaticRTTI<bool>() &&
        pPropRtti != ezGetStaticRTTI<ezInt64>() && pPropRtti != ezGetStaticRTTI<ezInt32>() && pPropRtti != ezGetStaticRTTI<ezInt16>() &&
        pPropRtti != ezGetStaticRTTI<ezInt8>() && pPropRtti != ezGetStaticRTTI<ezUInt64>() && pPropRtti != ezGetStaticRTTI<ezUInt32>() &&
        pPropRtti != ezGetStaticRTTI<ezUInt16>() && pPropRtti != ezGetStaticRTTI<ezUInt8>() && pPropRtti != ezGetStaticRTTI<ezAngle>() &&
        pPropRtti != ezGetStaticRTTI<ezTime>())
      return;
  }
  else if (pAnim->m_Target >= ezPropertyAnimTarget::VectorX && pAnim->m_Target <= ezPropertyAnimTarget::VectorW)
  {
    if (pPropRtti != ezGetStaticRTTI<ezVec2>() && pPropRtti != ezGetStaticRTTI<ezVec3>() && pPropRtti != ezGetStaticRTTI<ezVec4>())
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

void ezPropertyAnimComponent::CreateColorPropertyBinding(const ezColorPropertyAnimEntry* pAnim, const ezRTTI* pOwnerRtti, void* pObject,
  const ezComponentHandle& hComponent)
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
  binding.m_pAnimation = pAnim; // we can store a direct pointer here, because our SharedPtr keeps the descriptor alive
  binding.m_pMemberProperty = pMember;
}

void ezPropertyAnimComponent::ApplyAnimations(const ezTime& tDiff)
{
  if (m_fSpeed == 0.0f || m_AnimDesc == nullptr)
    return;

  const ezTime fLookupPos = ComputeAnimationLookup(tDiff);

  for (ezUInt32 i = 0; i < m_ComponentFloatBindings.GetCount();)
  {
    const auto& binding = m_ComponentFloatBindings[i];

    // if we have a component handle, use it to check that the component is still alive
    if (!binding.m_hComponent.IsInvalidated())
    {
      ezComponent* pComponent;
      if (!GetWorld()->TryGetComponent(binding.m_hComponent, pComponent))
      {
        // remove dead references
        m_ComponentFloatBindings.RemoveAtAndSwap(i);
        continue;
      }

      binding.m_pObject = static_cast<void*>(pComponent);
    }

    ApplyFloatAnimation(m_ComponentFloatBindings[i], fLookupPos);

    ++i;
  }

  for (ezUInt32 i = 0; i < m_ColorBindings.GetCount();)
  {
    const auto& binding = m_ColorBindings[i];

    // if we have a component handle, use it to check that the component is still alive
    if (!binding.m_hComponent.IsInvalidated())
    {
      ezComponent* pComponent;
      if (!GetWorld()->TryGetComponent(binding.m_hComponent, pComponent))

      {
        // remove dead references
        m_ColorBindings.RemoveAtAndSwap(i);
        continue;
      }

      binding.m_pObject = static_cast<void*>(pComponent);
    }

    ApplyColorAnimation(m_ColorBindings[i], fLookupPos);

    ++i;
  }

  for (ezUInt32 i = 0; i < m_GoFloatBindings.GetCount();)
  {
    const auto& binding = m_GoFloatBindings[i];

    // if we have a game object handle, use it to check that the component is still alive
    if (!binding.m_hObject.IsInvalidated())
    {
      ezGameObject* pObject;
      if (!GetWorld()->TryGetObject(binding.m_hObject, pObject))
      {
        // remove dead references
        m_GoFloatBindings.RemoveAtAndSwap(i);
        continue;
      }

      binding.m_pObject = static_cast<void*>(pObject);
    }

    ApplyFloatAnimation(m_GoFloatBindings[i], fLookupPos);

    ++i;
  }
}

ezTime ezPropertyAnimComponent::ComputeAnimationLookup(ezTime tDiff)
{
  const ezTime duration = m_AnimationRangeHigh - m_AnimationRangeLow;

  if (duration.IsZero())
    return m_AnimationRangeLow;

  tDiff = m_fSpeed * tDiff;

  ezMsgAnimationReachedEnd reachedEndMsg;
  ezTime tStart = m_AnimationTime;

  if (m_AnimationMode == ezPropertyAnimMode::Once)
  {
    m_AnimationTime += tDiff;

    if (m_fSpeed > 0 && m_AnimationTime >= m_AnimationRangeHigh)
    {
      m_AnimationTime = m_AnimationRangeHigh;
      m_bPlaying = false;

      m_ReachedEndMsgSender.SendMessage(reachedEndMsg, this, GetOwner());
    }
    else if (m_fSpeed < 0 && m_AnimationTime <= m_AnimationRangeLow)
    {
      m_AnimationTime = m_AnimationRangeLow;
      m_bPlaying = false;

      m_ReachedEndMsgSender.SendMessage(reachedEndMsg, this, GetOwner());
    }

    EvaluateEventTrack(tStart, m_AnimationTime);
  }
  else if (m_AnimationMode == ezPropertyAnimMode::Loop)
  {
    m_AnimationTime += tDiff;

    while (m_AnimationTime > m_AnimationRangeHigh)
    {
      m_AnimationTime -= duration;

      m_ReachedEndMsgSender.SendMessage(reachedEndMsg, this, GetOwner());

      EvaluateEventTrack(tStart, m_AnimationRangeHigh);
      tStart = m_AnimationRangeLow;
    }

    while (m_AnimationTime < m_AnimationRangeLow)
    {
      m_AnimationTime += duration;

      m_ReachedEndMsgSender.SendMessage(reachedEndMsg, this, GetOwner());

      EvaluateEventTrack(tStart, m_AnimationRangeLow);
      tStart = m_AnimationRangeHigh;
    }

    EvaluateEventTrack(tStart, m_AnimationTime);
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

        EvaluateEventTrack(tStart, m_AnimationRangeHigh);
        tStart = m_AnimationRangeHigh;

        m_ReachedEndMsgSender.SendMessage(reachedEndMsg, this, GetOwner());
      }
      else if (m_AnimationTime < m_AnimationRangeLow)
      {
        m_AnimationTime = m_AnimationRangeLow + (m_AnimationRangeLow - m_AnimationTime);
        m_bReverse = false;

        EvaluateEventTrack(tStart, m_AnimationRangeLow);
        tStart = m_AnimationRangeLow;

        m_ReachedEndMsgSender.SendMessage(reachedEndMsg, this, GetOwner());
      }
      else
      {
        EvaluateEventTrack(tStart, m_AnimationTime);
        break;
      }
    }
  }

  return m_AnimationTime;
}

void ezPropertyAnimComponent::EvaluateEventTrack(ezTime startTime, ezTime endTime)
{
  const ezEventTrack& et = m_AnimDesc->m_EventTrack;

  if (et.IsEmpty())
    return;

  ezHybridArray<ezHashedString, 8> events;
  et.Sample(startTime, endTime, events);

  for (const ezHashedString& sEvent : events)
  {
    ezMsgGenericEvent msg;
    msg.m_sMessage = sEvent.GetString();
    m_EventTrackMsgSender.SendMessage(msg, this, GetOwner());
  }
}

void ezPropertyAnimComponent::OnSimulationStarted()
{
  CreatePropertyBindings();

  StartPlayback();
}

void ezPropertyAnimComponent::StartPlayback()
{
  if (m_AnimDesc == nullptr)
    return;

  m_AnimationRangeLow = ezMath::Clamp(m_AnimationRangeLow, ezTime::Zero(), m_AnimDesc->m_AnimationDuration);
  m_AnimationRangeHigh = ezMath::Clamp(m_AnimationRangeHigh, m_AnimationRangeLow, m_AnimDesc->m_AnimationDuration);

  // when starting with a negative speed, start at the end of the animation and play backwards
  // important for play-once mode
  if (m_fSpeed < 0.0f)
  {
    m_AnimationTime = m_AnimationRangeHigh;
  }

  if (!m_RandomOffset.IsZero() && m_AnimDesc->m_AnimationDuration.IsPositive())
  {
    // should the random offset also be scaled by the speed factor? I guess not
    m_AnimationTime +=
      ezMath::Abs(m_fSpeed) * ezTime::Seconds(GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, m_RandomOffset.GetSeconds()));

    const ezTime duration = m_AnimationRangeHigh - m_AnimationRangeLow;

    if (duration.IsZeroOrNegative())
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

void ezPropertyAnimComponent::ApplySingleFloatAnimation(const FloatBinding& binding, ezTime lookupTime)
{
  const ezRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  double fFinalValue = 0;
  {
    const ezCurve1D& curve = binding.m_pAnimation[0]->m_Curve;

    if (curve.IsEmpty())
      return;

    fFinalValue = curve.Evaluate(lookupTime.GetSeconds());
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

void ezPropertyAnimComponent::ApplyFloatAnimation(const FloatBinding& binding, ezTime lookupTime)
{
  if (binding.m_pAnimation[0] != nullptr && binding.m_pAnimation[0]->m_Target == ezPropertyAnimTarget::Number)
  {
    ApplySingleFloatAnimation(binding, lookupTime);
    return;
  }

  const ezRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  float fCurValue[4] = {0, 0, 0, 0};

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
        fCurValue[i] = (float)curve.Evaluate(lookupTime.GetSeconds());
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

void ezPropertyAnimComponent::ApplyColorAnimation(const ColorBinding& binding, ezTime lookupTime)
{
  const ezRTTI* pRtti = binding.m_pMemberProperty->GetSpecificType();

  if (pRtti == ezGetStaticRTTI<ezColorGammaUB>())
  {
    ezColorGammaUB gamma;
    float intensity;
    binding.m_pAnimation->m_Gradient.Evaluate(lookupTime.AsFloatInSeconds(), gamma, intensity);
    binding.m_pMemberProperty->SetValuePtr(binding.m_pObject, &gamma);
    return;
  }

  if (pRtti == ezGetStaticRTTI<ezColor>())
  {
    ezColorGammaUB gamma;
    float intensity;
    binding.m_pAnimation->m_Gradient.Evaluate(lookupTime.AsFloatInSeconds(), gamma, intensity);

    ezColor finalColor = gamma;
    finalColor.ScaleRGB(intensity);
    binding.m_pMemberProperty->SetValuePtr(binding.m_pObject, &finalColor);
    return;
  }
}

void ezPropertyAnimComponent::Update()
{
  if (m_bPlaying == false || !m_hPropertyAnim.IsValid())
    return;

  if (m_AnimDesc == nullptr)
  {
    CreatePropertyBindings();
  }

  ApplyAnimations(GetWorld()->GetClock().GetTimeDiff());
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Components_Implementation_PropertyAnimComponent);
