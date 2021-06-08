#include <RendererCorePCH.h>

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

ezResult ezAnimGraphPin::Serialize(ezStreamWriter& stream) const
{
  stream << m_iPinIndex;
  stream << m_uiNumConnections;
  return EZ_SUCCESS;
}

ezResult ezAnimGraphPin::Deserialize(ezStreamReader& stream)
{
  stream >> m_iPinIndex;
  stream >> m_uiNumConnections;
  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphTriggerInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphTriggerInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphTriggerOutputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphTriggerOutputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezAnimGraphTriggerOutputPin::SetTriggered(ezAnimGraph& graph, bool triggered)
{
  if (m_iPinIndex < 0)
    return;

  if (!triggered)
    return;

  const auto& map = graph.m_OutputPinToInputPinMapping[ezAnimGraphPin::Trigger][m_iPinIndex];


  const ezInt8 offset = triggered ? +1 : -1;

  // trigger or reset all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    graph.m_TriggerInputPinStates[idx] += offset;
  }
}

bool ezAnimGraphTriggerInputPin::IsTriggered(ezAnimGraph& graph) const
{
  if (m_iPinIndex < 0)
    return false;

  return graph.m_TriggerInputPinStates[m_iPinIndex] > 0;
}

bool ezAnimGraphTriggerInputPin::AreAllTriggered(ezAnimGraph& graph) const
{
  return graph.m_TriggerInputPinStates[m_iPinIndex] == m_uiNumConnections;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphNumberInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphNumberInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphNumberOutputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphNumberOutputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

double ezAnimGraphNumberInputPin::GetNumber(ezAnimGraph& graph, double fFallback /*= 0.0*/) const
{
  if (m_iPinIndex < 0)
    return fFallback;

  return graph.m_NumberInputPinStates[m_iPinIndex];
}

void ezAnimGraphNumberOutputPin::SetNumber(ezAnimGraph& graph, double value)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = graph.m_OutputPinToInputPinMapping[ezAnimGraphPin::Number][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    graph.m_NumberInputPinStates[idx] = value;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphBoneWeightsInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphBoneWeightsInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphBoneWeightsOutputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphBoneWeightsOutputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimGraphPinDataBoneWeights* ezAnimGraphBoneWeightsInputPin::GetWeights(ezAnimGraph& graph) const
{
  if (m_iPinIndex < 0 || graph.m_BoneWeightInputPinStates[m_iPinIndex] == 0xFFFF)
    return nullptr;

  return &graph.m_PinDataBoneWeights[graph.m_BoneWeightInputPinStates[m_iPinIndex]];
}

void ezAnimGraphBoneWeightsOutputPin::SetWeights(ezAnimGraph& graph, ezAnimGraphPinDataBoneWeights* pWeights)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = graph.m_OutputPinToInputPinMapping[ezAnimGraphPin::BoneWeights][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    graph.m_BoneWeightInputPinStates[idx] = pWeights->m_uiOwnIndex;
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

ezAnimGraphPinDataLocalTransforms* ezAnimGraphLocalPoseInputPin::GetPose(ezAnimGraph& graph) const
{
  if (m_iPinIndex < 0)
    return nullptr;

  if (graph.m_LocalPoseInputPinStates[m_iPinIndex].IsEmpty())
    return nullptr;

  return &graph.m_PinDataLocalTransforms[graph.m_LocalPoseInputPinStates[m_iPinIndex][0]];
}

void ezAnimGraphLocalPoseMultiInputPin::GetPoses(ezAnimGraph& graph, ezDynamicArray<ezAnimGraphPinDataLocalTransforms*>& out_Poses) const
{
  out_Poses.Clear();

  if (m_iPinIndex < 0)
    return;

  out_Poses.SetCountUninitialized(graph.m_LocalPoseInputPinStates[m_iPinIndex].GetCount());
  for (ezUInt32 i = 0; i < graph.m_LocalPoseInputPinStates[m_iPinIndex].GetCount(); ++i)
  {
    out_Poses[i] = &graph.m_PinDataLocalTransforms[graph.m_LocalPoseInputPinStates[m_iPinIndex][i]];
  }
}

void ezAnimGraphLocalPoseOutputPin::SetPose(ezAnimGraph& graph, ezAnimGraphPinDataLocalTransforms* pPose)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = graph.m_OutputPinToInputPinMapping[ezAnimGraphPin::LocalPose][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    graph.m_LocalPoseInputPinStates[idx].PushBack(pPose->m_uiOwnIndex);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphModelPoseInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphModelPoseInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphModelPoseOutputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphModelPoseOutputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimGraphPinDataModelTransforms* ezAnimGraphModelPoseInputPin::GetPose(ezAnimGraph& graph) const
{
  if (m_iPinIndex < 0 || graph.m_ModelPoseInputPinStates[m_iPinIndex] == 0xFFFF)
    return nullptr;

  return &graph.m_PinDataModelTransforms[graph.m_ModelPoseInputPinStates[m_iPinIndex]];
}

void ezAnimGraphModelPoseOutputPin::SetPose(ezAnimGraph& graph, ezAnimGraphPinDataModelTransforms* pPose)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = graph.m_OutputPinToInputPinMapping[ezAnimGraphPin::ModelPose][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    graph.m_ModelPoseInputPinStates[idx] = pPose->m_uiOwnIndex;
  }
}
