#include <PCH.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptLogicNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Sequence, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Sequence>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic")
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_PROPERTIES
  {
    // Execution Pins (Input)
    EZ_INPUT_EXECUTION_PIN("run", 0),
    // Execution Pins (Output)
    EZ_OUTPUT_EXECUTION_PIN("then1", 0),
    EZ_OUTPUT_EXECUTION_PIN("then2", 1),
    EZ_OUTPUT_EXECUTION_PIN("then3", 2),
    EZ_OUTPUT_EXECUTION_PIN("then4", 3),
    EZ_OUTPUT_EXECUTION_PIN("then5", 4),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Sequence::ezVisualScriptNode_Sequence() { }

void ezVisualScriptNode_Sequence::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  pInstance->ExecuteConnectedNodes(this, 0);
  pInstance->ExecuteConnectedNodes(this, 1);
  pInstance->ExecuteConnectedNodes(this, 2);
  pInstance->ExecuteConnectedNodes(this, 3);
  pInstance->ExecuteConnectedNodes(this, 4);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Compare, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Compare>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    // Execution Pins
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("OnTrue", 0),
    EZ_OUTPUT_EXECUTION_PIN("OnFalse", 1),
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value1", 0, ezVisualScriptDataPinType::Number, m_Value1),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value2", 1, ezVisualScriptDataPinType::Number, m_Value2),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::Boolean),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Compare::ezVisualScriptNode_Compare()
{
  m_Value1 = 0.0;
  m_Value2 = 0.0;
}

void ezVisualScriptNode_Compare::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_Value1 < m_Value2)
  {
    bool result = true;
    pInstance->SetOutputPinValue(this, 0, &result);
    pInstance->ExecuteConnectedNodes(this, 0);
  }
  else
  {
    bool result = false;
    pInstance->SetOutputPinValue(this, 0, &result);
    pInstance->ExecuteConnectedNodes(this, 1);
  }
}

void* ezVisualScriptNode_Compare::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
  case 0:
    return &m_Value1;

  case 1:
    return &m_Value2;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////


