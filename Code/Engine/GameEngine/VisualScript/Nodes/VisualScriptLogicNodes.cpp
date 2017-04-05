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

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezLogicOperator, 1)
EZ_ENUM_CONSTANTS(ezLogicOperator::Equal, ezLogicOperator::Unequal, ezLogicOperator::Less, ezLogicOperator::LessEqual, ezLogicOperator::Greater, ezLogicOperator::GreaterEqual)
EZ_END_STATIC_REFLECTED_ENUM()

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Compare, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Compare>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    // Properties
    EZ_ENUM_MEMBER_PROPERTY("Operator", ezLogicOperator, m_Operator),

    // Data Pins (Input)
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value1", 0, ezVisualScriptDataPinType::Number, m_Value1),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value2", 1, ezVisualScriptDataPinType::Number, m_Value2),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::Boolean),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Compare::ezVisualScriptNode_Compare() { }
ezVisualScriptNode_Compare::~ezVisualScriptNode_Compare() { }

void ezVisualScriptNode_Compare::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  bool result = true;

  switch (m_Operator)
  {
  case ezLogicOperator::Equal:
    result = m_Value1 == m_Value2;
    break;
  case ezLogicOperator::Unequal:
    result = m_Value1 != m_Value2;
    break;
  case ezLogicOperator::Less:
    result = m_Value1 < m_Value2;
    break;
  case ezLogicOperator::LessEqual:
    result = m_Value1 <= m_Value2;
    break;
  case ezLogicOperator::Greater:
    result = m_Value1 > m_Value2;
    break;
  case ezLogicOperator::GreaterEqual:
    result = m_Value1 >= m_Value2;
    break;
  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  pInstance->SetOutputPinValue(this, 0, &result);
  pInstance->ExecuteConnectedNodes(this, result ? 0 : 1);
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_If, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_If>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    // Properties
    EZ_ENUM_MEMBER_PROPERTY("Operator", ezLogicOperator, m_Operator),
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

ezVisualScriptNode_If::ezVisualScriptNode_If() { }
ezVisualScriptNode_If::~ezVisualScriptNode_If() { }

void ezVisualScriptNode_If::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  bool result = true;

  switch (m_Operator)
  {
  case ezLogicOperator::Equal:
    result = m_Value1 == m_Value2;
    break;
  case ezLogicOperator::Unequal:
    result = m_Value1 != m_Value2;
    break;
  case ezLogicOperator::Less:
    result = m_Value1 < m_Value2;
    break;
  case ezLogicOperator::LessEqual:
    result = m_Value1 <= m_Value2;
    break;
  case ezLogicOperator::Greater:
    result = m_Value1 > m_Value2;
    break;
  case ezLogicOperator::GreaterEqual:
    result = m_Value1 >= m_Value2;
    break;
  default:
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  pInstance->SetOutputPinValue(this, 0, &result);
  pInstance->ExecuteConnectedNodes(this, result ? 0 : 1);
}

void* ezVisualScriptNode_If::GetInputPinDataPointer(ezUInt8 uiPin)
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Logic, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Logic>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN_AND_PROPERTY("a", 0, ezVisualScriptDataPinType::Boolean, m_Value1),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("b", 1, ezVisualScriptDataPinType::Boolean, m_Value2),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("AorB", 0, ezVisualScriptDataPinType::Boolean),
    EZ_OUTPUT_DATA_PIN("AandB", 0, ezVisualScriptDataPinType::Boolean),
    EZ_OUTPUT_DATA_PIN("AxorB", 0, ezVisualScriptDataPinType::Boolean),
    EZ_OUTPUT_DATA_PIN("notA", 0, ezVisualScriptDataPinType::Boolean),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Logic::ezVisualScriptNode_Logic() { }
ezVisualScriptNode_Logic::~ezVisualScriptNode_Logic() { }

void ezVisualScriptNode_Logic::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  const bool Or = m_Value1 || m_Value2;
  const bool And = m_Value1 && m_Value2;
  const bool Xor = m_Value1 ^ m_Value2;
  const bool NotA = !m_Value1;

  pInstance->SetOutputPinValue(this, 0, &Or);
  pInstance->SetOutputPinValue(this, 1, &And);
  pInstance->SetOutputPinValue(this, 2, &Xor);
  pInstance->SetOutputPinValue(this, 3, &NotA);
}

void* ezVisualScriptNode_Logic::GetInputPinDataPointer(ezUInt8 uiPin)
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



