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

void ezAnimGraphTriggerOutputPin::SetTriggered(ezAnimGraph& controller, bool triggered)
{
  if (m_iPinIndex < 0)
    return;

  if (!triggered)
    return;

  const auto& map = controller.m_OutputPinToInputPinMapping[ezAnimGraphPin::Trigger][m_iPinIndex];


  const ezInt8 offset = triggered ? +1 : -1;

  // trigger or reset all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    controller.m_TriggerInputPinStates[idx] += offset;
  }
}

bool ezAnimGraphTriggerInputPin::IsTriggered(ezAnimGraph& controller) const
{
  if (m_iPinIndex < 0)
    return false;

  return controller.m_TriggerInputPinStates[m_iPinIndex] > 0;
}

bool ezAnimGraphTriggerInputPin::AreAllTriggered(ezAnimGraph& controller) const
{
  return controller.m_TriggerInputPinStates[m_iPinIndex] == m_uiNumConnections;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphNumberInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphNumberInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphNumberOutputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphNumberOutputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

double ezAnimGraphNumberInputPin::GetNumber(ezAnimGraph& controller) const
{
  if (m_iPinIndex < 0)
    return 0;

  return controller.m_NumberInputPinStates[m_iPinIndex];
}

void ezAnimGraphNumberOutputPin::SetNumber(ezAnimGraph& controller, double value)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = controller.m_OutputPinToInputPinMapping[ezAnimGraphPin::Number][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    controller.m_NumberInputPinStates[idx] = value;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphBoneWeightsInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphBoneWeightsInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphBoneWeightsOutputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphBoneWeightsOutputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimGraphBoneWeights* ezAnimGraphBoneWeightsInputPin::GetWeights(ezAnimGraph& controller) const
{
  if (m_iPinIndex < 0)
    return nullptr;

  return controller.m_BoneWeightInputPinStates[m_iPinIndex];
}

void ezAnimGraphBoneWeightsOutputPin::SetWeights(ezAnimGraph& controller, ezAnimGraphBoneWeights* pWeights)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = controller.m_OutputPinToInputPinMapping[ezAnimGraphPin::BoneWeights][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    controller.m_BoneWeightInputPinStates[idx] = pWeights;
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

ezAnimGraphLocalTransforms* ezAnimGraphLocalPoseInputPin::GetPose(ezAnimGraph& controller) const
{
  if (m_iPinIndex < 0)
    return nullptr;

  if (controller.m_LocalPoseInputPinStates[m_iPinIndex].IsEmpty())
    return nullptr;

  return controller.m_LocalPoseInputPinStates[m_iPinIndex][0];
}

void ezAnimGraphLocalPoseMultiInputPin::GetPoses(ezAnimGraph& controller, ezDynamicArray<ezAnimGraphLocalTransforms*>& out_Poses) const
{
  out_Poses.Clear();

  if (m_iPinIndex < 0)
    return;

  out_Poses = controller.m_LocalPoseInputPinStates[m_iPinIndex];
}

void ezAnimGraphLocalPoseOutputPin::SetPose(ezAnimGraph& controller, ezAnimGraphLocalTransforms* pPose)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = controller.m_OutputPinToInputPinMapping[ezAnimGraphPin::LocalPose][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    controller.m_LocalPoseInputPinStates[idx].PushBack(pPose);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphModelPoseInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphModelPoseInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphModelPoseOutputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphModelPoseOutputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimGraphModelTransforms* ezAnimGraphModelPoseInputPin::GetPose(ezAnimGraph& controller) const
{
  if (m_iPinIndex < 0)
    return nullptr;

  return controller.m_ModelPoseInputPinStates[m_iPinIndex];
}

void ezAnimGraphModelPoseOutputPin::SetPose(ezAnimGraph& controller, ezAnimGraphModelTransforms* pPose)
{
  if (m_iPinIndex < 0)
    return;

  const auto& map = controller.m_OutputPinToInputPinMapping[ezAnimGraphPin::ModelPose][m_iPinIndex];

  // set all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    controller.m_ModelPoseInputPinStates[idx] = pPose;
  }
}
