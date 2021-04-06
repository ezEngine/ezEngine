#include <RendererCorePCH.h>

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
    EZ_MEMBER_PROPERTY("SetOnDeactivation", m_bSetOnDeactivation)->AddAttributes(new ezDefaultValueAttribute(false)),
    EZ_MEMBER_PROPERTY("DeactivationValue", m_fOnDeactivatedValue)->AddAttributes(new ezDefaultValueAttribute(0.0f)),

    EZ_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Set: '{BlackboardEntry}' '{ActivationValue}' '{DeactivationValue}'"),
    new ezCategoryAttribute("Blackboard"),
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
  stream << m_fOnDeactivatedValue;
  stream << m_bSetOnActivation;
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
  stream >> m_fOnDeactivatedValue;
  stream >> m_bSetOnActivation;
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
  const bool bIsActiveNow = m_ActivePin.IsTriggered(graph);

  if (bIsActiveNow != m_bLastActiveState)
  {
    m_bLastActiveState = bIsActiveNow;

    // TODO: register only once
    graph.GetBlackboard().RegisterEntry(m_sBlackboardEntry, 0.0f);

    if (bIsActiveNow)
    {
      if (m_bSetOnActivation)
      {
        graph.GetBlackboard().SetEntryValue(m_sBlackboardEntry, m_fOnActivatedValue);
      }
    }
    else
    {
      if (m_bSetOnDeactivation)
      {
        graph.GetBlackboard().SetEntryValue(m_sBlackboardEntry, m_fOnDeactivatedValue);
      }
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
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezComparisonOperator, 1)
  EZ_ENUM_CONSTANTS(ezComparisonOperator::Equal, ezComparisonOperator::NotEqual)
  EZ_ENUM_CONSTANTS(ezComparisonOperator::Less, ezComparisonOperator::LessEqual)
  EZ_ENUM_CONSTANTS(ezComparisonOperator::Greater, ezComparisonOperator::GreaterEqual)
EZ_END_STATIC_REFLECTED_ENUM;

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

static bool Compare(ezComparisonOperator::Enum cmp, float f1, float f2)
{
  switch (cmp)
  {
    case ezComparisonOperator::Equal:
      return f1 == f2;
    case ezComparisonOperator::NotEqual:
      return f1 != f2;
    case ezComparisonOperator::Less:
      return f1 < f2;
    case ezComparisonOperator::LessEqual:
      return f1 <= f2;
    case ezComparisonOperator::Greater:
      return f1 > f2;
    case ezComparisonOperator::GreaterEqual:
      return f1 >= f2;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return false;
}

void ezCheckBlackboardValueAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  ezVariant value = graph.GetBlackboard().GetEntryValue(m_sBlackboardEntry);
  float fValue = 0.0f;

  if (value.IsValid() && value.IsNumber())
  {
    fValue = value.ConvertTo<float>();
  }

  if (Compare(m_Comparison, fValue, m_fReferenceValue))
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
  ezVariant value = graph.GetBlackboard().GetEntryValue(m_sBlackboardEntry);
  double fValue = 0.0f;

  if (value.IsValid() && value.IsNumber())
  {
    fValue = value.ConvertTo<double>();
  }

  m_NumberPin.SetNumber(graph, fValue);
}
