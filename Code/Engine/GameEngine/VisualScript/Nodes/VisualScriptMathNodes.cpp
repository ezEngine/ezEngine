#include <PCH.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptMathNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_MultiplyAdd, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_MultiplyAdd>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Math")
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN_AND_PROPERTY("a1", 0, ezVisualScriptDataPinType::Number, m_Value1),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("a2", 1, ezVisualScriptDataPinType::Number, m_Value2)->AddAttributes(new ezDefaultValueAttribute(1.0)),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("b1", 1, ezVisualScriptDataPinType::Number, m_Value3),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("b2", 1, ezVisualScriptDataPinType::Number, m_Value4)->AddAttributes(new ezDefaultValueAttribute(1.0)),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_MultiplyAdd::ezVisualScriptNode_MultiplyAdd() { }
ezVisualScriptNode_MultiplyAdd::~ezVisualScriptNode_MultiplyAdd() { }

void ezVisualScriptNode_MultiplyAdd::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  const double result = m_Value1 * m_Value2 + m_Value3 * m_Value4;
  pInstance->SetOutputPinValue(this, 0, &result);
}

void* ezVisualScriptNode_MultiplyAdd::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
  case 0:
    return &m_Value1;
  case 1:
    return &m_Value2;
  case 2:
    return &m_Value3;
  case 3:
    return &m_Value4;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Div, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Div>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Math")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN_AND_PROPERTY("a", 0, ezVisualScriptDataPinType::Number, m_Value1)->AddAttributes(new ezDefaultValueAttribute(1.0)),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("b", 1, ezVisualScriptDataPinType::Number, m_Value2)->AddAttributes(new ezDefaultValueAttribute(1.0)),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Div::ezVisualScriptNode_Div() { }
ezVisualScriptNode_Div::~ezVisualScriptNode_Div() { }

void ezVisualScriptNode_Div::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  const double result = m_Value1 / m_Value2;
  pInstance->SetOutputPinValue(this, 0, &result);
}

void* ezVisualScriptNode_Div::GetInputPinDataPointer(ezUInt8 uiPin)
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



