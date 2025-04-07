#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/BlackboardAnimNodes.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSetBlackboardNumberAnimNode, 1, ezRTTIDefaultAllocator<ezSetBlackboardNumberAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry)->AddAttributes(new ezDynamicStringEnumAttribute("BlackboardKeysEnum")),
    EZ_MEMBER_PROPERTY("Number", m_fNumber),

    EZ_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("InNumber", m_InNumber)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Set Number: '{BlackboardEntry}' to {Number}"),
    new ezCategoryAttribute("Blackboard"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Red)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezSetBlackboardNumberAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_fNumber;

  EZ_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InNumber.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezSetBlackboardNumberAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_fNumber;

  EZ_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InNumber.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezSetBlackboardNumberAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* ezSetBlackboardNumberAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void ezSetBlackboardNumberAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (m_InActivate.IsConnected() && !m_InActivate.IsTriggered(ref_graph))
    return;

  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  pBlackboard->SetEntryValue(m_sBlackboardEntry, m_InNumber.GetNumber(ref_graph, m_fNumber));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGetBlackboardNumberAnimNode, 1, ezRTTIDefaultAllocator<ezGetBlackboardNumberAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry)->AddAttributes(new ezDynamicStringEnumAttribute("BlackboardKeysEnum")),

    EZ_MEMBER_PROPERTY("OutNumber", m_OutNumber)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Blackboard"),
    new ezTitleAttribute("Get Number: '{BlackboardEntry}'"),
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

  EZ_SUCCEED_OR_RETURN(m_OutNumber.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezGetBlackboardNumberAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  EZ_SUCCEED_OR_RETURN(m_OutNumber.Deserialize(stream));

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

void ezGetBlackboardNumberAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  ezVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.IsNumber())
  {
    ezLog::Warning("AnimController::GetBlackboardNumber: '{}' doesn't exist or isn't a number type.", m_sBlackboardEntry);
    return;
  }

  m_OutNumber.SetNumber(ref_graph, value.ConvertTo<double>());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCompareBlackboardNumberAnimNode, 1, ezRTTIDefaultAllocator<ezCompareBlackboardNumberAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry)->AddAttributes(new ezDynamicStringEnumAttribute("BlackboardKeysEnum")),
    EZ_MEMBER_PROPERTY("ReferenceValue", m_fReferenceValue),
    EZ_ENUM_MEMBER_PROPERTY("Comparison", ezComparisonOperator, m_Comparison),

    EZ_MEMBER_PROPERTY("OutOnTrue", m_OutOnTrue)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("OutOnFalse", m_OutOnFalse)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new ezHiddenAttribute()),
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

ezResult ezCompareBlackboardNumberAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(2);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_fReferenceValue;
  stream << m_Comparison;

  EZ_SUCCEED_OR_RETURN(m_OutOnTrue.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnFalse.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezCompareBlackboardNumberAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(2);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_fReferenceValue;
  stream >> m_Comparison;

  EZ_SUCCEED_OR_RETURN(m_OutOnTrue.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnFalse.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezCompareBlackboardNumberAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* ezCompareBlackboardNumberAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void ezCompareBlackboardNumberAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  const ezVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.IsNumber())
  {
    ezLog::Warning("AnimController::CompareBlackboardNumber: '{}' doesn't exist or isn't a number type.", m_sBlackboardEntry);
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const double fValue = value.ConvertTo<double>();
  const bool bIsTrueNow = ezComparisonOperator::Compare(m_Comparison, fValue, m_fReferenceValue);
  const ezInt8 iIsTrueNow = bIsTrueNow ? 1 : 0;

  m_OutIsTrue.SetBool(ref_graph, bIsTrueNow);

  // we use a tri-state bool here to ensure that OnTrue or OnFalse get fired right away
  if (pInstance->m_iIsTrue != iIsTrueNow)
  {
    pInstance->m_iIsTrue = iIsTrueNow;

    if (bIsTrueNow)
    {
      m_OutOnTrue.SetTriggered(ref_graph);
    }
    else
    {
      m_OutOnFalse.SetTriggered(ref_graph);
    }
  }
}

bool ezCompareBlackboardNumberAnimNode::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCheckBlackboardBoolAnimNode, 1, ezRTTIDefaultAllocator<ezCheckBlackboardBoolAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry)->AddAttributes(new ezDynamicStringEnumAttribute("BlackboardKeysEnum")),

    EZ_MEMBER_PROPERTY("OutOnTrue", m_OutOnTrue)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("OutOnFalse", m_OutOnFalse)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("OutBool", m_OutBool)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Blackboard"),
    new ezTitleAttribute("Check Bool: '{BlackboardEntry}'"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Lime)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezCheckBlackboardBoolAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  EZ_SUCCEED_OR_RETURN(m_OutOnTrue.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnFalse.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutBool.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezCheckBlackboardBoolAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  EZ_SUCCEED_OR_RETURN(m_OutOnTrue.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnFalse.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutBool.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezCheckBlackboardBoolAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* ezCheckBlackboardBoolAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void ezCheckBlackboardBoolAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  const ezVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.CanConvertTo<bool>())
  {
    ezLog::Warning("AnimController::CheckBlackboardBool: '{}' doesn't exist or isn't a bool type.", m_sBlackboardEntry);
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  const bool bValue = value.ConvertTo<bool>();
  const ezInt8 iIsTrueNow = bValue ? 1 : 0;

  m_OutBool.SetBool(ref_graph, bValue);

  // we use a tri-state bool here to ensure that OnTrue or OnFalse get fired right away
  if (pInstance->m_iIsTrue != iIsTrueNow)
  {
    pInstance->m_iIsTrue = iIsTrueNow;

    if (bValue)
    {
      m_OutOnTrue.SetTriggered(ref_graph);
    }
    else
    {
      m_OutOnFalse.SetTriggered(ref_graph);
    }
  }
}

bool ezCheckBlackboardBoolAnimNode::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSetBlackboardBoolAnimNode, 1, ezRTTIDefaultAllocator<ezSetBlackboardBoolAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry)->AddAttributes(new ezDynamicStringEnumAttribute("BlackboardKeysEnum")),
    EZ_MEMBER_PROPERTY("Bool", m_bBool),

    EZ_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Set Bool: '{BlackboardEntry}' to {Bool}"),
    new ezCategoryAttribute("Blackboard"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Red)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezSetBlackboardBoolAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;
  stream << m_bBool;

  EZ_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InBool.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezSetBlackboardBoolAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;
  stream >> m_bBool;

  EZ_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InBool.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezSetBlackboardBoolAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* ezSetBlackboardBoolAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void ezSetBlackboardBoolAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  pBlackboard->SetEntryValue(m_sBlackboardEntry, m_InBool.GetBool(ref_graph, m_bBool));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGetBlackboardBoolAnimNode, 1, ezRTTIDefaultAllocator<ezGetBlackboardBoolAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry)->AddAttributes(new ezDynamicStringEnumAttribute("BlackboardKeysEnum")),

    EZ_MEMBER_PROPERTY("OutBool", m_OutBool)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Blackboard"),
    new ezTitleAttribute("Get Bool: '{BlackboardEntry}'"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Lime)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezGetBlackboardBoolAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  EZ_SUCCEED_OR_RETURN(m_OutBool.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezGetBlackboardBoolAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  EZ_SUCCEED_OR_RETURN(m_OutBool.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezGetBlackboardBoolAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* ezGetBlackboardBoolAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void ezGetBlackboardBoolAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  ezVariant value = pBlackboard->GetEntryValue(m_sBlackboardEntry);

  if (!value.IsValid() || !value.CanConvertTo<bool>())
  {
    ezLog::Warning("AnimController::GetBlackboardBool: '{}' doesn't exist or can't be converted to bool.", m_sBlackboardEntry);
    return;
  }

  m_OutBool.SetBool(ref_graph, value.ConvertTo<bool>());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezOnBlackboardValueChangedAnimNode, 1, ezRTTIDefaultAllocator<ezOnBlackboardValueChangedAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("BlackboardEntry", GetBlackboardEntry, SetBlackboardEntry)->AddAttributes(new ezDynamicStringEnumAttribute("BlackboardKeysEnum")),

    EZ_MEMBER_PROPERTY("OutOnValueChanged", m_OutOnValueChanged)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Blackboard"),
    new ezTitleAttribute("OnChanged: '{BlackboardEntry}'"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Lime)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezOnBlackboardValueChangedAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sBlackboardEntry;

  EZ_SUCCEED_OR_RETURN(m_OutOnValueChanged.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezOnBlackboardValueChangedAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sBlackboardEntry;

  EZ_SUCCEED_OR_RETURN(m_OutOnValueChanged.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezOnBlackboardValueChangedAnimNode::SetBlackboardEntry(const char* szFile)
{
  m_sBlackboardEntry.Assign(szFile);
}

const char* ezOnBlackboardValueChangedAnimNode::GetBlackboardEntry() const
{
  return m_sBlackboardEntry.GetData();
}

void ezOnBlackboardValueChangedAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  auto pBlackboard = ref_controller.GetBlackboard();
  if (pBlackboard == nullptr)
    return;

  if (m_sBlackboardEntry.IsEmpty())
    return;

  const ezBlackboard::Entry* pEntry = pBlackboard->GetEntry(m_sBlackboardEntry);

  if (pEntry == nullptr)
  {
    ezLog::Warning("AnimController::OnBlackboardValueChanged: '{}' doesn't exist.", m_sBlackboardEntry);
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  if (pInstance->m_uiChangeCounter == pEntry->m_uiChangeCounter)
    return;

  if (pInstance->m_uiChangeCounter != ezInvalidIndex)
  {
    m_OutOnValueChanged.SetTriggered(ref_graph);
  }

  pInstance->m_uiChangeCounter = pEntry->m_uiChangeCounter;
}

bool ezOnBlackboardValueChangedAnimNode::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

//////////////////////////////////////////////////////////////////////////


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_BlackboardAnimNodes);
