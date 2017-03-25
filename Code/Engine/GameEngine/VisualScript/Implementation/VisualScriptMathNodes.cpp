#include <PCH.h>
#include <GameEngine/VisualScript/Implementation/VisualScriptMathNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Add, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Add>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Math")
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_CONSTANT_PROPERTY("Value1", 0)->AddAttributes(new ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory::Number)),
    EZ_CONSTANT_PROPERTY("Value2", 1)->AddAttributes(new ezVisualScriptInputPinAttribute(ezVisualScriptPinCategory::Number)),
    // Data Pins (Output)
    EZ_CONSTANT_PROPERTY("Sum", 0)->AddAttributes(new ezVisualScriptOutputPinAttribute(ezVisualScriptPinCategory::Number)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Add::ezVisualScriptNode_Add() { }

void ezVisualScriptNode_Add::Execute(ezVisualScriptInstance* pInstance)
{
  double result = m_Value1 + m_Value2;
  pInstance->SetOutputPinValue(this, 0, result);
}

void ezVisualScriptNode_Add::SetInputPinValue(ezUInt8 uiPin, const ezVariant& value)
{
  switch (uiPin)
  {
  case 0:
    m_Value1 = value.ConvertTo<double>();
    return;
  case 1:
    m_Value2 = value.ConvertTo<double>();
    return;
  }
}

//////////////////////////////////////////////////////////////////////////

