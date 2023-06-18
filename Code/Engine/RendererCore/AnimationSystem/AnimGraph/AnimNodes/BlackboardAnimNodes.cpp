#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/BlackboardAnimNodes.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSetBlackboardValueAnimNode, 1, ezRTTIDefaultAllocator<ezSetBlackboardValueAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),
    EZ_MEMBER_PROPERTY("SetOnActivation", m_bSetOnActivation)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("ActivationValue", m_fOnActivatedValue)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("SetOnHold", m_bSetOnHold)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("HoldValue", m_fOnHoldValue)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("SetOnDeactivation", m_bSetOnDeactivation)->AddAttributes(new ezDefaultValueAttribute(false)),
    EZ_MEMBER_PROPERTY("DeactivationValue", m_fOnDeactivatedValue)->AddAttributes(new ezDefaultValueAttribute(0.0f)),

    EZ_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Set: '{BlackboardEntry}' '{ActivationValue}''"),
    new ezCategoryAttribute("Blackboard"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Red)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezSetBlackboardValueAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_fOnActivatedValue;
  stream << m_fOnHoldValue;
  stream << m_fOnDeactivatedValue;
  stream << m_bSetOnActivation;
  stream << m_bSetOnHold;
  stream << m_bSetOnDeactivation;

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezSetBlackboardValueAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_fOnActivatedValue;
  stream >> m_fOnHoldValue;
  stream >> m_fOnDeactivatedValue;
  stream >> m_bSetOnActivation;
  stream >> m_bSetOnHold;
  stream >> m_bSetOnDeactivation;

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezSetBlackboardValueAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* ezSetBlackboardValueAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void ezSetBlackboardValueAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  auto pBlackboard = graph.GetBlackboard();
  if (pBlackboard == nullptr)
  {
    ezLog::Warning("No blackboard available for the animation controller graph to use.");
    return;
  }

  const bool bIsActiveNow = m_ActivePin.IsTriggered(graph);

  if (bIsActiveNow != m_bLastActiveState)
  {
    m_bLastActiveState = bIsActiveNow;

    if (bIsActiveNow)
    {
      if (m_bSetOnActivation)
      {
        if (pBlackboard->SetEntryValue(m_sBlackboardEntry, m_fOnActivatedValue).Failed())
        {
          ezLog::Warning("Can't set blackboard value '{}', it isn't known.", m_sBlackboardEntry);
        }
      }
    }
    else
    {
      if (m_bSetOnDeactivation)
      {
        if (pBlackboard->SetEntryValue(m_sBlackboardEntry, m_fOnDeactivatedValue).Failed())
        {
          ezLog::Warning("Can't set blackboard value '{}', it isn't known.", m_sBlackboardEntry);
        }
      }
    }
  }
  else if (bIsActiveNow && m_bSetOnHold)
  {
    if (pBlackboard->SetEntryValue(m_sBlackboardEntry, m_fOnHoldValue).Failed())
    {
      ezLog::Warning("Can't set blackboard value '{}', it isn't known.", m_sBlackboardEntry);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCheckBlackboardValueAnimNode, 1, ezRTTIDefaultAllocator<ezCheckBlackboardValueAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),
    EZ_MEMBER_PROPERTY("ReferenceValue", m_fReferenceValue)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_ENUM_MEMBER_PROPERTY("Comparison", ezComparisonOperator, m_Comparison),

    EZ_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Blackboard"),
    new ezTitleAttribute("Check: '{BlackboardEntry}' {Comparison} {ReferenceValue}"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Lime)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezCheckBlackboardValueAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_fReferenceValue;
  stream << m_Comparison;

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezCheckBlackboardValueAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_fReferenceValue;
  stream >> m_Comparison;

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezCheckBlackboardValueAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* ezCheckBlackboardValueAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void ezCheckBlackboardValueAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  auto pBlackboard = graph.GetBlackboard();
  if (pBlackboard == nullptr)
  {
    ezLog::Warning("No blackboard available for the animation controller graph to use.");
    return;
  }

  float fValue = 0.0f;
  if (!m_sBlackboardEntry.IsEmpty())
  {
    ezVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

    if (value.IsValid() && value.IsNumber())
    {
      fValue = value.ConvertTo<float>();
    }
    else
    {
      ezLog::Warning("Blackboard entry '{}' doesn't exist.", m_sBlackboardEntry);
      return;
    }
  }

  if (ezComparisonOperator::Compare(m_Comparison, fValue, m_fReferenceValue))
  {
    m_ActivePin.SetTriggered(graph, true);
  }
  else
  {
    m_ActivePin.SetTriggered(graph, false);
  }
}

//////////////////////////////////////////////////////////////////////////


// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGetBlackboardNumberAnimNode, 1, ezRTTIDefaultAllocator<ezGetBlackboardNumberAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry),

    EZ_MEMBER_PROPERTY("Number", m_NumberPin)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Blackboard"),
    new ezTitleAttribute("Get: '{BlackboardEntry}'"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Lime)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezGetBlackboardNumberAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  EZ_SUCCEED_OR_RETURN(m_NumberPin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezGetBlackboardNumberAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  EZ_SUCCEED_OR_RETURN(m_NumberPin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezGetBlackboardNumberAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* ezGetBlackboardNumberAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void ezGetBlackboardNumberAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  auto pBlackboard = graph.GetBlackboard();
  if (pBlackboard == nullptr)
  {
    ezLog::Warning("No blackboard available for the animation controller graph to use.");
    return;
  }

  double fValue = 0.0f;

  if (!m_sBlackboardEntry.IsEmpty())
  {
    ezVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

    if (value.IsValid() && value.IsNumber())
    {
      fValue = value.ConvertTo<double>();
    }
    else
    {
      ezLog::Warning("Blackboard entry '{}' doesn't exist.", m_sBlackboardEntry);
      return;
    }
  }

  m_NumberPin.SetNumber(graph, fValue);
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_BlackboardAnimNodes);
