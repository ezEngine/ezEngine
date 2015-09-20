#include <PCH.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Memory/MemoryUtils.h>

EZ_CREATE_SIMPLE_TEST_GROUP(Input);

static bool operator== (const ezInputActionConfig& lhs, const ezInputActionConfig& rhs)
{
  if (lhs.m_bApplyTimeScaling != rhs.m_bApplyTimeScaling)
    return false;
  if (lhs.m_fFilteredPriority != rhs.m_fFilteredPriority)
    return false;
  if (lhs.m_fFilterXMaxValue != rhs.m_fFilterXMaxValue)
    return false;
  if (lhs.m_fFilterXMinValue != rhs.m_fFilterXMinValue)
    return false;
  if (lhs.m_fFilterYMaxValue != rhs.m_fFilterYMaxValue)
    return false;
  if (lhs.m_fFilterYMinValue != rhs.m_fFilterYMinValue)
    return false;

  if (lhs.m_OnEnterArea != rhs.m_OnEnterArea)
    return false;
  if (lhs.m_OnLeaveArea != rhs.m_OnLeaveArea)
    return false;

  for (int i = 0; i < ezInputActionConfig::MaxInputSlotAlternatives; ++i)
  {
    if (lhs.m_sInputSlotTrigger[i] != rhs.m_sInputSlotTrigger[i])
      return false;
    if (lhs.m_fInputSlotScale[i] != rhs.m_fInputSlotScale[i])
      return false;
    if (lhs.m_sFilterByInputSlotX[i] != rhs.m_sFilterByInputSlotX[i])
      return false;
    if (lhs.m_sFilterByInputSlotY[i] != rhs.m_sFilterByInputSlotY[i])
      return false;
  }
  
  return true;
}

class ezTestInputDevide : public ezInputDevice
{
public:

  void ActivateAll()
  {
    m_InputSlotValues["testdevice_button"] = 0.1f;
    m_InputSlotValues["testdevice_stick"] = 0.2f;
    m_InputSlotValues["testdevice_wheel"] = 0.3f;
    m_InputSlotValues["testdevice_touchpoint"] = 0.4f;
    m_LastCharacter = '\42';
  }

private:

  void InitializeDevice() override { }
  void UpdateInputSlotValues() override { }
  void RegisterInputSlots() override
  {
    RegisterInputSlot("testdevice_button", "", ezInputSlotFlags::IsButton);
    RegisterInputSlot("testdevice_stick", "", ezInputSlotFlags::IsAnalogStick);
    RegisterInputSlot("testdevice_wheel", "", ezInputSlotFlags::IsMouseWheel);
    RegisterInputSlot("testdevice_touchpoint", "", ezInputSlotFlags::IsTouchPoint);
  }

  void ResetInputSlotValues() override
  {
    m_InputSlotValues.Clear();
  }
};

static bool operator!= (const ezInputActionConfig& lhs, const ezInputActionConfig& rhs)
{
  return !(lhs == rhs);
}

EZ_CREATE_SIMPLE_TEST(Input, InputManager)
{
  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetInputSlotDisplayName / GetInputSlotDisplayName")
  {
    ezInputManager::SetInputSlotDisplayName("test_slot_1", "Test Slot 1 Name");
    ezInputManager::SetInputSlotDisplayName("test_slot_2", "Test Slot 2 Name");
    ezInputManager::SetInputSlotDisplayName("test_slot_3", "Test Slot 3 Name");
    ezInputManager::SetInputSlotDisplayName("test_slot_4", "Test Slot 4 Name");

    EZ_TEST_BOOL(ezStringUtils::IsEqual(ezInputManager::GetInputSlotDisplayName("test_slot_1"), "Test Slot 1 Name"));
    EZ_TEST_BOOL(ezStringUtils::IsEqual(ezInputManager::GetInputSlotDisplayName("test_slot_2"), "Test Slot 2 Name"));
    EZ_TEST_BOOL(ezStringUtils::IsEqual(ezInputManager::GetInputSlotDisplayName("test_slot_3"), "Test Slot 3 Name"));
    EZ_TEST_BOOL(ezStringUtils::IsEqual(ezInputManager::GetInputSlotDisplayName("test_slot_4"), "Test Slot 4 Name"));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetInputSlotDeadZone / GetInputSlotDisplayName")
  {
    ezInputManager::SetInputSlotDeadZone("test_slot_1", 0.1f);
    ezInputManager::SetInputSlotDeadZone("test_slot_2", 0.2f);
    ezInputManager::SetInputSlotDeadZone("test_slot_3", 0.3f);
    ezInputManager::SetInputSlotDeadZone("test_slot_4", 0.4f);

    EZ_TEST_FLOAT(ezInputManager::GetInputSlotDeadZone("test_slot_1"), 0.1f, 0.0f);
    EZ_TEST_FLOAT(ezInputManager::GetInputSlotDeadZone("test_slot_2"), 0.2f, 0.0f);
    EZ_TEST_FLOAT(ezInputManager::GetInputSlotDeadZone("test_slot_3"), 0.3f, 0.0f);
    EZ_TEST_FLOAT(ezInputManager::GetInputSlotDeadZone("test_slot_4"), 0.4f, 0.0f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetInputActionConfig / GetInputActionConfig")
  {
    ezInputActionConfig iac1, iac2;
    iac1.m_bApplyTimeScaling = true;
    iac1.m_fFilteredPriority = 23.0f;
    iac1.m_fInputSlotScale[0] = 2.0f;
    iac1.m_fInputSlotScale[1] = 3.0f;
    iac1.m_fInputSlotScale[2] = 4.0f;
    iac1.m_sInputSlotTrigger[0] = ezInputSlot_Key0;
    iac1.m_sInputSlotTrigger[1] = ezInputSlot_Key1;
    iac1.m_sInputSlotTrigger[2] = ezInputSlot_Key2;

    iac2.m_bApplyTimeScaling = false;
    iac2.m_fFilteredPriority = 42.0f;
    iac2.m_fInputSlotScale[0] = 4.0f;
    iac2.m_fInputSlotScale[1] = 5.0f;
    iac2.m_fInputSlotScale[2] = 6.0f;
    iac2.m_sInputSlotTrigger[0] = ezInputSlot_Key3;
    iac2.m_sInputSlotTrigger[1] = ezInputSlot_Key4;
    iac2.m_sInputSlotTrigger[2] = ezInputSlot_Key5;

    ezInputManager::SetInputActionConfig("test_inputset", "test_action_1", iac1, true);
    ezInputManager::SetInputActionConfig("test_inputset", "test_action_2", iac2, true);

    EZ_TEST_BOOL(ezInputManager::GetInputActionConfig("test_inputset", "test_action_1") == iac1);
    EZ_TEST_BOOL(ezInputManager::GetInputActionConfig("test_inputset", "test_action_2") == iac2);

    ezInputManager::SetInputActionConfig("test_inputset", "test_action_3", iac1, false);
    ezInputManager::SetInputActionConfig("test_inputset", "test_action_4", iac2, false);

    EZ_TEST_BOOL(ezInputManager::GetInputActionConfig("test_inputset", "test_action_1") == iac1);
    EZ_TEST_BOOL(ezInputManager::GetInputActionConfig("test_inputset", "test_action_2") == iac2);
    EZ_TEST_BOOL(ezInputManager::GetInputActionConfig("test_inputset", "test_action_3") == iac1);
    EZ_TEST_BOOL(ezInputManager::GetInputActionConfig("test_inputset", "test_action_4") == iac2);

    ezInputManager::SetInputActionConfig("test_inputset", "test_action_3", iac1, true);
    ezInputManager::SetInputActionConfig("test_inputset", "test_action_4", iac2, true);

    EZ_TEST_BOOL(ezInputManager::GetInputActionConfig("test_inputset", "test_action_1") != iac1);
    EZ_TEST_BOOL(ezInputManager::GetInputActionConfig("test_inputset", "test_action_2") != iac2);
    EZ_TEST_BOOL(ezInputManager::GetInputActionConfig("test_inputset", "test_action_3") == iac1);
    EZ_TEST_BOOL(ezInputManager::GetInputActionConfig("test_inputset", "test_action_4") == iac2);


    ezInputManager::RemoveInputAction("test_inputset", "test_action_1");
    ezInputManager::RemoveInputAction("test_inputset", "test_action_2");
    ezInputManager::RemoveInputAction("test_inputset", "test_action_3");
    ezInputManager::RemoveInputAction("test_inputset", "test_action_4");


  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Input Slot State Changes / Dead Zones")
  {
    float f = 0;
    ezInputManager::InjectInputSlotValue("test_slot_1", 0.0f);
    ezInputManager::SetInputSlotDeadZone("test_slot_1", 0.25f);

    // just check the first state
    EZ_TEST_BOOL(ezInputManager::GetInputSlotState("test_slot_1", &f) == ezKeyState::Up);
    EZ_TEST_FLOAT(f, 0.0f, 0);

    // value is not yet propagated
    ezInputManager::InjectInputSlotValue("test_slot_1", 1.0f);
    EZ_TEST_BOOL(ezInputManager::GetInputSlotState("test_slot_1", &f) == ezKeyState::Up);
    EZ_TEST_FLOAT(f, 0.0f, 0);

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));
    EZ_TEST_BOOL(ezInputManager::GetInputSlotState("test_slot_1", &f) == ezKeyState::Pressed);
    EZ_TEST_FLOAT(f, 1.0f, 0);

    ezInputManager::InjectInputSlotValue("test_slot_1", 0.5f);

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));
    EZ_TEST_BOOL(ezInputManager::GetInputSlotState("test_slot_1", &f) == ezKeyState::Down);
    EZ_TEST_FLOAT(f, 0.5f, 0);

    ezInputManager::InjectInputSlotValue("test_slot_1", 0.3f);

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));
    EZ_TEST_BOOL(ezInputManager::GetInputSlotState("test_slot_1", &f) == ezKeyState::Down);
    EZ_TEST_FLOAT(f, 0.3f, 0);

    // below dead zone value
    ezInputManager::InjectInputSlotValue("test_slot_1", 0.2f);

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));
    EZ_TEST_BOOL(ezInputManager::GetInputSlotState("test_slot_1", &f) == ezKeyState::Released);
    EZ_TEST_FLOAT(f, 0.0f, 0);

    ezInputManager::InjectInputSlotValue("test_slot_1", 0.5f);

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));
    EZ_TEST_BOOL(ezInputManager::GetInputSlotState("test_slot_1", &f) == ezKeyState::Pressed);
    EZ_TEST_FLOAT(f, 0.5f, 0);

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));
    EZ_TEST_BOOL(ezInputManager::GetInputSlotState("test_slot_1", &f) == ezKeyState::Released);
    EZ_TEST_FLOAT(f, 0.0f, 0);

    ezInputManager::InjectInputSlotValue("test_slot_1", 0.2f);

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));
    EZ_TEST_BOOL(ezInputManager::GetInputSlotState("test_slot_1", &f) == ezKeyState::Up);
    EZ_TEST_FLOAT(f, 0.0f, 0);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "SetActionDisplayName / GetActionDisplayName")
  {
    ezInputManager::SetActionDisplayName("test_action_1", "Test Action 1 Name");
    ezInputManager::SetActionDisplayName("test_action_2", "Test Action 2 Name");
    ezInputManager::SetActionDisplayName("test_action_3", "Test Action 3 Name");
    ezInputManager::SetActionDisplayName("test_action_4", "Test Action 4 Name");

    EZ_TEST_BOOL(ezStringUtils::IsEqual(ezInputManager::GetActionDisplayName("test_action_1"), "Test Action 1 Name"));
    EZ_TEST_BOOL(ezStringUtils::IsEqual(ezInputManager::GetActionDisplayName("test_action_2"), "Test Action 2 Name"));
    EZ_TEST_BOOL(ezStringUtils::IsEqual(ezInputManager::GetActionDisplayName("test_action_3"), "Test Action 3 Name"));
    EZ_TEST_BOOL(ezStringUtils::IsEqual(ezInputManager::GetActionDisplayName("test_action_4"), "Test Action 4 Name"));
  }


  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Input Sets")
  {
    ezInputActionConfig iac;
    ezInputManager::SetInputActionConfig("test_inputset", "test_action_1", iac, true);
    ezInputManager::SetInputActionConfig("test_inputset2", "test_action_2", iac, true);

    ezDynamicArray<ezString> InputSetNames;
    ezInputManager::GetAllInputSets(InputSetNames);

    EZ_TEST_INT(InputSetNames.GetCount(), 2);

    EZ_TEST_STRING(InputSetNames[0].GetData(), "test_inputset");
    EZ_TEST_STRING(InputSetNames[1].GetData(), "test_inputset2");

    ezInputManager::RemoveInputAction("test_inputset", "test_action_1");
    ezInputManager::RemoveInputAction("test_inputset2", "test_action_2");
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetAllInputActions / RemoveInputAction")
  {
    ezDynamicArray<ezString> InputActions;

    ezInputManager::GetAllInputActions("test_inputset_3", InputActions);

    EZ_TEST_BOOL(InputActions.IsEmpty());

    ezInputActionConfig iac;
    ezInputManager::SetInputActionConfig("test_inputset_3", "test_action_1", iac, true);
    ezInputManager::SetInputActionConfig("test_inputset_3", "test_action_2", iac, true);
    ezInputManager::SetInputActionConfig("test_inputset_3", "test_action_3", iac, true);

    ezInputManager::GetAllInputActions("test_inputset_3", InputActions);

    EZ_TEST_INT(InputActions.GetCount(), 3);

    EZ_TEST_STRING(InputActions[0].GetData(), "test_action_1");
    EZ_TEST_STRING(InputActions[1].GetData(), "test_action_2");
    EZ_TEST_STRING(InputActions[2].GetData(), "test_action_3");


    ezInputManager::RemoveInputAction("test_inputset_3", "test_action_2");

    ezInputManager::GetAllInputActions("test_inputset_3", InputActions);

    EZ_TEST_INT(InputActions.GetCount(), 2);

    EZ_TEST_STRING(InputActions[0].GetData(), "test_action_1");
    EZ_TEST_STRING(InputActions[1].GetData(), "test_action_3");

    ezInputManager::RemoveInputAction("test_inputset_3", "test_action_1");
    ezInputManager::RemoveInputAction("test_inputset_3", "test_action_3");

    ezInputManager::GetAllInputActions("test_inputset_3", InputActions);

    EZ_TEST_BOOL(InputActions.IsEmpty());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Input Action State Changes")
  {
    ezInputActionConfig iac;
    iac.m_bApplyTimeScaling = false;
    iac.m_sInputSlotTrigger[0] = "test_input_slot_1";
    iac.m_sInputSlotTrigger[1] = "test_input_slot_2";
    iac.m_sInputSlotTrigger[2] = "test_input_slot_3";
    
    // bind the three slots to this action
    ezInputManager::SetInputActionConfig("test_inputset", "test_action", iac, true);

    // bind the same three slots to another action
    ezInputManager::SetInputActionConfig("test_inputset", "test_action_2", iac, false);

    // the first slot to trigger the action is bound to it, the other slots can now trigger other actions
    // but not this one anymore
    ezInputManager::InjectInputSlotValue("test_input_slot_2", 1.0f);

    float f = 0;
    ezInt8 iSlot = 0;
    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == ezKeyState::Up);
    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == ezKeyState::Up);

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));

    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == ezKeyState::Pressed);
    EZ_TEST_INT(iSlot, 1);
    EZ_TEST_FLOAT(f, 1.0f, 0.0f);

    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == ezKeyState::Pressed);
    EZ_TEST_INT(iSlot, 1);
    EZ_TEST_FLOAT(f, 1.0f, 0.0f);

    // inject all three input slots
    ezInputManager::InjectInputSlotValue("test_input_slot_1", 1.0f);
    ezInputManager::InjectInputSlotValue("test_input_slot_2", 1.0f);
    ezInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));

    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == ezKeyState::Down);
    EZ_TEST_INT(iSlot, 1); // still the same slot that 'triggered' the action
    EZ_TEST_FLOAT(f, 1.0f, 0.0f);

    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == ezKeyState::Down);
    EZ_TEST_INT(iSlot, 1); // still the same slot that 'triggered' the action
    EZ_TEST_FLOAT(f, 1.0f, 0.0f);

    ezInputManager::InjectInputSlotValue("test_input_slot_1", 1.0f);
    ezInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));

    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == ezKeyState::Released);
    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == ezKeyState::Released);

    ezInputManager::InjectInputSlotValue("test_input_slot_1", 1.0f);
    ezInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));

    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == ezKeyState::Up);
    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == ezKeyState::Up);

    ezInputManager::InjectInputSlotValue("test_input_slot_3", 1.0f);

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));

    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == ezKeyState::Pressed);
    EZ_TEST_INT(iSlot, 2);
    EZ_TEST_FLOAT(f, 1.0f, 0.0f);

    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == ezKeyState::Pressed);
    EZ_TEST_INT(iSlot, 2);
    EZ_TEST_FLOAT(f, 1.0f, 0.0f);

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));

    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == ezKeyState::Released);
    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == ezKeyState::Released);

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));

    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action", &f, &iSlot) == ezKeyState::Up);
    EZ_TEST_BOOL(ezInputManager::GetInputActionState("test_inputset", "test_action_2", &f, &iSlot) == ezKeyState::Up);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetPressedInputSlot")
  {
    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));

    const char* szSlot = ezInputManager::GetPressedInputSlot(ezInputSlotFlags::None, ezInputSlotFlags::None);
    EZ_TEST_BOOL(ezStringUtils::IsNullOrEmpty(szSlot));

    ezInputManager::InjectInputSlotValue("test_slot", 1.0f);

    szSlot = ezInputManager::GetPressedInputSlot(ezInputSlotFlags::None, ezInputSlotFlags::None);
    EZ_TEST_BOOL(ezStringUtils::IsEqual(szSlot, ""));

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));

    szSlot = ezInputManager::GetPressedInputSlot(ezInputSlotFlags::None, ezInputSlotFlags::None);
    EZ_TEST_BOOL(ezStringUtils::IsEqual(szSlot, "test_slot"));


    {
      ezTestInputDevide dev;
      dev.ActivateAll();

      ezInputManager::InjectInputSlotValue("test_slot", 1.0f);

      ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));

      szSlot = ezInputManager::GetPressedInputSlot(ezInputSlotFlags::IsButton, ezInputSlotFlags::None);
      EZ_TEST_BOOL(ezStringUtils::IsEqual(szSlot, "testdevice_button"));

      szSlot = ezInputManager::GetPressedInputSlot(ezInputSlotFlags::IsAnalogStick, ezInputSlotFlags::None);
      EZ_TEST_BOOL(ezStringUtils::IsEqual(szSlot, "testdevice_stick"));

      szSlot = ezInputManager::GetPressedInputSlot(ezInputSlotFlags::IsMouseWheel, ezInputSlotFlags::None);
      EZ_TEST_BOOL(ezStringUtils::IsEqual(szSlot, "testdevice_wheel"));

      szSlot = ezInputManager::GetPressedInputSlot(ezInputSlotFlags::IsTouchPoint, ezInputSlotFlags::None);
      EZ_TEST_BOOL(ezStringUtils::IsEqual(szSlot, "testdevice_touchpoint"));

      ezInputManager::InjectInputSlotValue("test_slot", 1.0f);

      ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));

      szSlot = ezInputManager::GetPressedInputSlot(ezInputSlotFlags::IsButton, ezInputSlotFlags::None);
      EZ_TEST_BOOL(ezStringUtils::IsEqual(szSlot, ""));

      szSlot = ezInputManager::GetPressedInputSlot(ezInputSlotFlags::IsAnalogStick, ezInputSlotFlags::None);
      EZ_TEST_BOOL(ezStringUtils::IsEqual(szSlot, ""));

      szSlot = ezInputManager::GetPressedInputSlot(ezInputSlotFlags::IsMouseWheel, ezInputSlotFlags::None);
      EZ_TEST_BOOL(ezStringUtils::IsEqual(szSlot, ""));

      szSlot = ezInputManager::GetPressedInputSlot(ezInputSlotFlags::IsTouchPoint, ezInputSlotFlags::None);
      EZ_TEST_BOOL(ezStringUtils::IsEqual(szSlot, ""));
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "LastCharacter")
  {
    ezTestInputDevide dev;
    dev.ActivateAll();

    EZ_TEST_BOOL(ezInputManager::RetrieveLastCharacter(true) == '\0');

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));

    EZ_TEST_BOOL(ezInputManager::RetrieveLastCharacter(false) == '\42');
    EZ_TEST_BOOL(ezInputManager::RetrieveLastCharacter(true) == '\42');
    EZ_TEST_BOOL(ezInputManager::RetrieveLastCharacter(true) == '\0');
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Time Scaling")
  {
    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));

    ezInputActionConfig iac;
    iac.m_bApplyTimeScaling = true;
    iac.m_sInputSlotTrigger[0] = "testdevice_button";
    ezInputManager::SetInputActionConfig("test_inputset", "test_timescaling", iac, true);

    ezTestInputDevide dev;
    dev.ActivateAll();

    float fVal;

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));
    ezInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    EZ_TEST_FLOAT(fVal, 0.1f * (1.0 / 60.0), 0.0001f); // testdevice_button has a value of 0.1f

    dev.ActivateAll();

    ezInputManager::Update(ezTime::Seconds(1.0 / 30.0));
    ezInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    EZ_TEST_FLOAT(fVal, 0.1f * (1.0 / 30.0), 0.0001f);


    iac.m_bApplyTimeScaling = false;
    iac.m_sInputSlotTrigger[0] = "testdevice_button";
    ezInputManager::SetInputActionConfig("test_inputset", "test_timescaling", iac, true);

    dev.ActivateAll();

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));
    ezInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    EZ_TEST_FLOAT(fVal, 0.1f, 0.0001f); // testdevice_button has a value of 0.1f

    dev.ActivateAll();

    ezInputManager::Update(ezTime::Seconds(1.0 / 30.0));
    ezInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    EZ_TEST_FLOAT(fVal, 0.1f, 0.0001f);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "GetInputSlotFlags")
  {
    ezTestInputDevide dev;
    ezInputManager::Update(ezTime::Seconds(1.0 / 30.0));

    EZ_TEST_BOOL(ezInputManager::GetInputSlotFlags("testdevice_button") == ezInputSlotFlags::IsButton);
    EZ_TEST_BOOL(ezInputManager::GetInputSlotFlags("testdevice_stick") == ezInputSlotFlags::IsAnalogStick);
    EZ_TEST_BOOL(ezInputManager::GetInputSlotFlags("testdevice_wheel") == ezInputSlotFlags::IsMouseWheel);
    EZ_TEST_BOOL(ezInputManager::GetInputSlotFlags("testdevice_touchpoint") == ezInputSlotFlags::IsTouchPoint);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ClearInputMapping")
  {
    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));

    ezInputActionConfig iac;
    iac.m_bApplyTimeScaling = true;
    iac.m_sInputSlotTrigger[0] = "testdevice_button";
    ezInputManager::SetInputActionConfig("test_inputset", "test_timescaling", iac, true);

    ezTestInputDevide dev;
    dev.ActivateAll();

    float fVal;

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));
    ezInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    EZ_TEST_FLOAT(fVal, 0.1f * (1.0 / 60.0), 0.0001f); // testdevice_button has a value of 0.1f

    // clear the button from the action
    ezInputManager::ClearInputMapping("test_inputset", "testdevice_button");

    dev.ActivateAll();

    ezInputManager::Update(ezTime::Seconds(1.0 / 60.0));

    // should not receive input anymore
    ezInputManager::GetInputActionState("test_inputset", "test_timescaling", &fVal);

    EZ_TEST_FLOAT(fVal, 0.0f, 0.0001f);
  }
}

