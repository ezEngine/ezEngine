#include <RendererCore/RendererCorePCH.h>

#include <Core/Input/InputManager.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/ControllerInputAnimNode.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezControllerInputAnimNode, 1, ezRTTIDefaultAllocator<ezControllerInputAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("LeftStickX", m_OutLeftStickX)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("LeftStickY", m_OutLeftStickY)->AddAttributes(new ezHiddenAttribute()),

    EZ_MEMBER_PROPERTY("RightStickX", m_OutRightStickX)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("RightStickY", m_OutRightStickY)->AddAttributes(new ezHiddenAttribute()),

    EZ_MEMBER_PROPERTY("LeftTrigger", m_OutLeftTrigger)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("RightTrigger", m_OutRightTrigger)->AddAttributes(new ezHiddenAttribute()),

    EZ_MEMBER_PROPERTY("ButtonA", m_OutButtonA)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("ButtonB", m_OutButtonB)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("ButtonX", m_OutButtonX)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("ButtonY", m_OutButtonY)->AddAttributes(new ezHiddenAttribute()),

    EZ_MEMBER_PROPERTY("LeftShoulder", m_OutLeftShoulder)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("RightShoulder", m_OutRightShoulder)->AddAttributes(new ezHiddenAttribute()),

    EZ_MEMBER_PROPERTY("PadLeft", m_OutPadLeft)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("PadRight", m_OutPadRight)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("PadUp", m_OutPadUp)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("PadDown", m_OutPadDown)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Input"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Pink)),
    new ezTitleAttribute("Controller"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezControllerInputAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_OutLeftStickX.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutLeftStickY.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutRightStickX.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutRightStickY.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutLeftTrigger.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutRightTrigger.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutButtonA.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutButtonB.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutButtonX.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutButtonY.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutLeftShoulder.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutRightShoulder.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPadLeft.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPadRight.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPadUp.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPadDown.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezControllerInputAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_OutLeftStickX.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutLeftStickY.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutRightStickX.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutRightStickY.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutLeftTrigger.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutRightTrigger.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutButtonA.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutButtonB.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutButtonX.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutButtonY.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutLeftShoulder.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutRightShoulder.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPadLeft.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPadRight.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPadUp.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutPadDown.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezControllerInputAnimNode::Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  {
    float fValue1 = 0.0f;
    float fValue2 = 0.0f;

    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_NegX, &fValue1);
    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_PosX, &fValue2);
    m_OutLeftStickX.SetNumber(graph, -fValue1 + fValue2);

    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_NegY, &fValue1);
    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_PosY, &fValue2);
    m_OutLeftStickY.SetNumber(graph, -fValue1 + fValue2);

    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_RightStick_NegX, &fValue1);
    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_RightStick_PosX, &fValue2);
    m_OutRightStickX.SetNumber(graph, -fValue1 + fValue2);

    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_RightStick_NegY, &fValue1);
    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_RightStick_PosY, &fValue2);
    m_OutRightStickY.SetNumber(graph, -fValue1 + fValue2);
  }

  {
    float fValue = 0.0f;
    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonA, &fValue);
    m_OutButtonA.SetBool(graph, fValue > 0);

    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonB, &fValue);
    m_OutButtonB.SetBool(graph, fValue > 0);

    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonX, &fValue);
    m_OutButtonX.SetBool(graph, fValue > 0);

    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonY, &fValue);
    m_OutButtonY.SetBool(graph, fValue > 0);

    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftShoulder, &fValue);
    m_OutLeftShoulder.SetBool(graph, fValue > 0);

    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftTrigger, &fValue);
    m_OutLeftTrigger.SetNumber(graph, fValue);

    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_RightShoulder, &fValue);
    m_OutRightShoulder.SetBool(graph, fValue > 0);

    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_RightTrigger, &fValue);
    m_OutRightTrigger.SetNumber(graph, fValue);

    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_PadLeft, &fValue);
    m_OutPadLeft.SetBool(graph, fValue > 0);

    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_PadRight, &fValue);
    m_OutPadRight.SetBool(graph, fValue > 0);

    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_PadUp, &fValue);
    m_OutPadUp.SetBool(graph, fValue > 0);

    ezInputManager::GetInputSlotState(ezInputSlot_Controller0_PadDown, &fValue);
    m_OutPadDown.SetBool(graph, fValue > 0);
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_ControllerInputAnimNode);
