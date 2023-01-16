#include <RendererCore/RendererCorePCH.h>

#include <Core/Input/InputManager.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/ControllerInputAnimNode.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezControllerInputAnimNode, 1, ezRTTIDefaultAllocator<ezControllerInputAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("LeftTrigger", m_LeftTrigger)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("LeftShoulder", m_LeftShoulder)->AddAttributes(new ezHiddenAttribute()),

    EZ_MEMBER_PROPERTY("LeftStickX", m_LeftStickX)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("LeftStickY", m_LeftStickY)->AddAttributes(new ezHiddenAttribute()),

    EZ_MEMBER_PROPERTY("PadLeft", m_PadLeft)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("PadRight", m_PadRight)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("PadUp", m_PadUp)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("PadDown", m_PadDown)->AddAttributes(new ezHiddenAttribute()),

    EZ_MEMBER_PROPERTY("RightTrigger", m_RightTrigger)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("RightShoulder", m_RightShoulder)->AddAttributes(new ezHiddenAttribute()),

    EZ_MEMBER_PROPERTY("RightStickX", m_RightStickX)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("RightStickY", m_RightStickY)->AddAttributes(new ezHiddenAttribute()),

    EZ_MEMBER_PROPERTY("ButtonA", m_ButtonA)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("ButtonB", m_ButtonB)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("ButtonX", m_ButtonX)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("ButtonY", m_ButtonY)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Input"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Pink)),
    new ezTitleAttribute("XBox Controller"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezControllerInputAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_ButtonA.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ButtonB.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ButtonX.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ButtonY.Serialize(stream));

  EZ_SUCCEED_OR_RETURN(m_LeftStickX.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LeftStickY.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_RightStickX.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_RightStickY.Serialize(stream));

  EZ_SUCCEED_OR_RETURN(m_LeftTrigger.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_RightTrigger.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LeftShoulder.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_RightShoulder.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_PadLeft.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_PadRight.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_PadUp.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_PadDown.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezControllerInputAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_ButtonA.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ButtonB.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ButtonX.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ButtonY.Deserialize(stream));

  EZ_SUCCEED_OR_RETURN(m_LeftStickX.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LeftStickY.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_RightStickX.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_RightStickY.Deserialize(stream));

  EZ_SUCCEED_OR_RETURN(m_LeftTrigger.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_RightTrigger.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_LeftShoulder.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_RightShoulder.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_PadLeft.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_PadRight.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_PadUp.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_PadDown.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezControllerInputAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  float fValue1 = 0.0f;
  float fValue2 = 0.0f;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_NegX, &fValue1);
  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_PosX, &fValue2);
  m_LeftStickX.SetNumber(graph, -fValue1 + fValue2);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_NegY, &fValue1);
  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_PosY, &fValue2);
  m_LeftStickY.SetNumber(graph, -fValue1 + fValue2);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_RightStick_NegX, &fValue1);
  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_RightStick_PosX, &fValue2);
  m_RightStickX.SetNumber(graph, -fValue1 + fValue2);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_RightStick_NegY, &fValue1);
  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_RightStick_PosY, &fValue2);
  m_RightStickY.SetNumber(graph, -fValue1 + fValue2);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonA, &fValue1);
  m_ButtonA.SetTriggered(graph, fValue1 > 0);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonB, &fValue1);
  m_ButtonB.SetTriggered(graph, fValue1 > 0);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonX, &fValue1);
  m_ButtonX.SetTriggered(graph, fValue1 > 0);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonY, &fValue1);
  m_ButtonY.SetTriggered(graph, fValue1 > 0);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftShoulder, &fValue1);
  m_LeftShoulder.SetTriggered(graph, fValue1 > 0);
  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftTrigger, &fValue1);
  m_LeftTrigger.SetNumber(graph, fValue1);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_RightShoulder, &fValue1);
  m_RightShoulder.SetTriggered(graph, fValue1 > 0);
  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_RightTrigger, &fValue1);
  m_RightTrigger.SetNumber(graph, fValue1);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_PadLeft, &fValue1);
  m_PadLeft.SetTriggered(graph, fValue1 > 0);
  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_PadRight, &fValue1);
  m_PadRight.SetTriggered(graph, fValue1 > 0);
  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_PadUp, &fValue1);
  m_PadUp.SetTriggered(graph, fValue1 > 0);
  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_PadDown, &fValue1);
  m_PadDown.SetTriggered(graph, fValue1 > 0);
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_ControllerInputAnimNode);

