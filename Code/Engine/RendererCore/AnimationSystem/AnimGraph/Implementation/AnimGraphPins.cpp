#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphPin, 1, ezRTTIDefaultAllocator<ezAnimGraphPin>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PinIdx", m_iPinIndex)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("NumConnections", m_uiNumConnections)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphOutputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphOutputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezAnimGraphPin::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream << m_iPinIndex;
  inout_stream << m_uiNumConnections;
  return EZ_SUCCESS;
}

ezResult ezAnimGraphPin::Deserialize(ezStreamReader& inout_stream)
{
  inout_stream >> m_iPinIndex;
  inout_stream >> m_uiNumConnections;
  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphTriggerInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphTriggerInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphTriggerOutputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphTriggerOutputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezAnimGraphTriggerOutputPin::SetTriggered(ezAnimGraph& ref_graph, bool bTriggered)
{
  if (m_iPinIndex < 0)
    return;

  if (!bTriggered)
    return;

  const auto& map = ref_graph.m_OutputPinToInputPinMapping[ezAnimGraphPin::Trigger][m_iPinIndex];


  const ezInt8 offset = +1; // bTriggered ? +1 : -1;

  // trigger or reset all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    ref_graph.m_TriggerInputPinStates[idx] += offset;
  }
}

bool ezAnimGraphTriggerInputPin::IsTriggered(ezAnimGraph& ref_graph) const
{
  if (m_iPinIndex < 0)
    return false;

  return ref_graph.m_TriggerInputPinStates[m_iPinIndex] > 0;
}

bool ezAnimGraphTriggerInputPin::AreAllTriggered(ezAnimGraph& ref_graph) const
{
  return ref_graph.m_TriggerInputPinStates[m_iPinIndex] == m_uiNumConnections;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphNumberInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphNumberInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphNumberOutputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphNumberOutputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

double ezAnimGraphNumberInputPin::GetNumber(ezAnimGraph& ref_graph, double fFallback /*= 0.0*/) const
{
  if (m_iPinIndex < 0)
    return fFallback;

  return ref_graph.m_NumberInputPinStates[m_iPinIndex];
}

void ezAnimGraphNumberOutputPin::SetNumber(ezAnimGraph& ref_graph, double value)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_OutputPinToInputPinMapping[ezAnimGraphPin::Number][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    ref_graph.m_NumberInputPinStates[idx] = value;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphBoneWeightsInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphBoneWeightsInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphBoneWeightsOutputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphBoneWeightsOutputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimGraphPinDataBoneWeights* ezAnimGraphBoneWeightsInputPin::GetWeights(ezAnimGraph& ref_graph) const
{
  if (m_iPinIndex < 0 || ref_graph.m_BoneWeightInputPinStates[m_iPinIndex] == 0xFFFF)
    return nullptr;

  return &ref_graph.m_PinDataBoneWeights[ref_graph.m_BoneWeightInputPinStates[m_iPinIndex]];
}

void ezAnimGraphBoneWeightsOutputPin::SetWeights(ezAnimGraph& ref_graph, ezAnimGraphPinDataBoneWeights* pWeights)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_OutputPinToInputPinMapping[ezAnimGraphPin::BoneWeights][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    ref_graph.m_BoneWeightInputPinStates[idx] = pWeights->m_uiOwnIndex;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphLocalPoseInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphLocalPoseInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphLocalPoseMultiInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphLocalPoseMultiInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphLocalPoseOutputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphLocalPoseOutputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimGraphPinDataLocalTransforms* ezAnimGraphLocalPoseInputPin::GetPose(ezAnimGraph& ref_graph) const
{
  if (m_iPinIndex < 0)
    return nullptr;

  if (ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].IsEmpty())
    return nullptr;

  return &ref_graph.m_PinDataLocalTransforms[ref_graph.m_LocalPoseInputPinStates[m_iPinIndex][0]];
}

void ezAnimGraphLocalPoseMultiInputPin::GetPoses(ezAnimGraph& ref_graph, ezDynamicArray<ezAnimGraphPinDataLocalTransforms*>& out_poses) const
{
  out_poses.Clear();

  if (m_iPinIndex < 0)
    return;

  out_poses.SetCountUninitialized(ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].GetCount());
  for (ezUInt32 i = 0; i < ref_graph.m_LocalPoseInputPinStates[m_iPinIndex].GetCount(); ++i)
  {
    out_poses[i] = &ref_graph.m_PinDataLocalTransforms[ref_graph.m_LocalPoseInputPinStates[m_iPinIndex][i]];
  }
}

void ezAnimGraphLocalPoseOutputPin::SetPose(ezAnimGraph& ref_graph, ezAnimGraphPinDataLocalTransforms* pPose)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_OutputPinToInputPinMapping[ezAnimGraphPin::LocalPose][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    ref_graph.m_LocalPoseInputPinStates[idx].PushBack(pPose->m_uiOwnIndex);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphModelPoseInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphModelPoseInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphModelPoseOutputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphModelPoseOutputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimGraphPinDataModelTransforms* ezAnimGraphModelPoseInputPin::GetPose(ezAnimGraph& ref_graph) const
{
  if (m_iPinIndex < 0 || ref_graph.m_ModelPoseInputPinStates[m_iPinIndex] == 0xFFFF)
    return nullptr;

  return &ref_graph.m_PinDataModelTransforms[ref_graph.m_ModelPoseInputPinStates[m_iPinIndex]];
}

void ezAnimGraphModelPoseOutputPin::SetPose(ezAnimGraph& ref_graph, ezAnimGraphPinDataModelTransforms* pPose)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = ref_graph.m_OutputPinToInputPinMapping[ezAnimGraphPin::ModelPose][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    ref_graph.m_ModelPoseInputPinStates[idx] = pPose->m_uiOwnIndex;
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphPins);
