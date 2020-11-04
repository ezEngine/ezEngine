#include <RendererCorePCH.h>

#include <Core/Input/InputManager.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/ControllerInputAnimNode.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezControllerInputAnimNode, 1, ezRTTIDefaultAllocator<ezControllerInputAnimNode>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("ButtonA", m_ButtonA)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("ButtonB", m_ButtonB)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("ButtonX", m_ButtonX)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("ButtonY", m_ButtonY)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("StickLeft", m_StickLeft)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("StickRight", m_StickRight)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("StickUp", m_StickUp)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("StickDown", m_StickDown)->AddAttributes(new ezHiddenAttribute()),
    }
    EZ_END_PROPERTIES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezControllerInputAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  m_ButtonA.Serialize(stream);
  m_ButtonB.Serialize(stream);
  m_ButtonX.Serialize(stream);
  m_ButtonY.Serialize(stream);

  m_StickLeft.Serialize(stream);
  m_StickRight.Serialize(stream);
  m_StickUp.Serialize(stream);
  m_StickDown.Serialize(stream);

  return EZ_SUCCESS;
}

ezResult ezControllerInputAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  m_ButtonA.Deserialize(stream);
  m_ButtonB.Deserialize(stream);
  m_ButtonX.Deserialize(stream);
  m_ButtonY.Deserialize(stream);

  m_StickLeft.Deserialize(stream);
  m_StickRight.Deserialize(stream);
  m_StickUp.Deserialize(stream);
  m_StickDown.Deserialize(stream);

  return EZ_SUCCESS;
}

void ezControllerInputAnimNode::Step(ezAnimGraph* pOwner, ezTime tDiff, const ezSkeletonResource* pSkeleton)
{
  float fValue = 0.0f;

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_NegX, &fValue);
  m_StickLeft.SetTriggered(*pOwner, fValue > 0);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_PosX, &fValue);
  m_StickRight.SetTriggered(*pOwner, fValue > 0);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_NegY, &fValue);
  m_StickDown.SetTriggered(*pOwner, fValue > 0);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_LeftStick_PosY, &fValue);
  m_StickUp.SetTriggered(*pOwner, fValue > 0);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonA, &fValue);
  m_ButtonA.SetTriggered(*pOwner, fValue > 0);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonB, &fValue);
  m_ButtonB.SetTriggered(*pOwner, fValue > 0);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonX, &fValue);
  m_ButtonX.SetTriggered(*pOwner, fValue > 0);

  ezInputManager::GetInputSlotState(ezInputSlot_Controller0_ButtonY, &fValue);
  m_ButtonY.SetTriggered(*pOwner, fValue > 0);
}
