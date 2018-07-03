#include <PCH.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptMathNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_MultiplyAdd, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_MultiplyAdd>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Math"),
    new ezTitleAttribute("'{a1}*{a2} + {b1}*{b2}'"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN_AND_PROPERTY("a1", 0, ezVisualScriptDataPinType::Number, m_Value1),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("a2", 1, ezVisualScriptDataPinType::Number, m_Value2)->AddAttributes(new ezDefaultValueAttribute(1.0)),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("b1", 2, ezVisualScriptDataPinType::Number, m_Value3),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("b2", 3, ezVisualScriptDataPinType::Number, m_Value4)->AddAttributes(new ezDefaultValueAttribute(1.0)),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualScriptNode_MultiplyAdd::ezVisualScriptNode_MultiplyAdd() { }
ezVisualScriptNode_MultiplyAdd::~ezVisualScriptNode_MultiplyAdd() { }

void ezVisualScriptNode_MultiplyAdd::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const double result = m_Value1 * m_Value2 + m_Value3 * m_Value4;
    pInstance->SetOutputPinValue(this, 0, &result);
  }
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
    new ezCategoryAttribute("Math"),
    new ezTitleAttribute("Div: {a} / {b}"),
  }
    EZ_END_ATTRIBUTES;
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN_AND_PROPERTY("a", 0, ezVisualScriptDataPinType::Number, m_Value1)->AddAttributes(new ezDefaultValueAttribute(1.0)),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("b", 1, ezVisualScriptDataPinType::Number, m_Value2)->AddAttributes(new ezDefaultValueAttribute(1.0)),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualScriptNode_Div::ezVisualScriptNode_Div() { }
ezVisualScriptNode_Div::~ezVisualScriptNode_Div() { }

void ezVisualScriptNode_Div::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const double result = m_Value1 / m_Value2;
    pInstance->SetOutputPinValue(this, 0, &result);
  }
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Min, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Min>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Math"),
    new ezTitleAttribute("Min ({a}, {b})"),
  }
    EZ_END_ATTRIBUTES;
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN_AND_PROPERTY("a", 0, ezVisualScriptDataPinType::Number, m_Value1),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("b", 1, ezVisualScriptDataPinType::Number, m_Value2),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualScriptNode_Min::ezVisualScriptNode_Min() { }
ezVisualScriptNode_Min::~ezVisualScriptNode_Min() { }

void ezVisualScriptNode_Min::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const double result = ezMath::Min(m_Value1, m_Value2);
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* ezVisualScriptNode_Min::GetInputPinDataPointer(ezUInt8 uiPin)
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Max, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Max>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Math"),
    new ezTitleAttribute("Max ({a}, {b})"),
  }
    EZ_END_ATTRIBUTES;
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN_AND_PROPERTY("a", 0, ezVisualScriptDataPinType::Number, m_Value1),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("b", 1, ezVisualScriptDataPinType::Number, m_Value2),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualScriptNode_Max::ezVisualScriptNode_Max() { }
ezVisualScriptNode_Max::~ezVisualScriptNode_Max() { }

void ezVisualScriptNode_Max::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const double result = ezMath::Max(m_Value1, m_Value2);
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* ezVisualScriptNode_Max::GetInputPinDataPointer(ezUInt8 uiPin)
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Clamp, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Clamp>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Math"),
    new ezTitleAttribute("Clamp ({Min}, {Max})"),
  }
    EZ_END_ATTRIBUTES;
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN("Value", 0, ezVisualScriptDataPinType::Number),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Min", 1, ezVisualScriptDataPinType::Number, m_MinValue),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Max", 2, ezVisualScriptDataPinType::Number, m_MaxValue),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualScriptNode_Clamp::ezVisualScriptNode_Clamp() { }
ezVisualScriptNode_Clamp::~ezVisualScriptNode_Clamp() { }

void ezVisualScriptNode_Clamp::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const double result = ezMath::Clamp(m_Value, m_MinValue, m_MaxValue);
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* ezVisualScriptNode_Clamp::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
  case 0:
    return &m_Value;
  case 1:
    return &m_MinValue;
  case 2:
    return &m_MaxValue;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Abs, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Abs>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Math")
  }
    EZ_END_ATTRIBUTES;
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN("Value", 0, ezVisualScriptDataPinType::Number),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualScriptNode_Abs::ezVisualScriptNode_Abs() { }
ezVisualScriptNode_Abs::~ezVisualScriptNode_Abs() { }

void ezVisualScriptNode_Abs::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const double result = ezMath::Abs(m_Value);
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* ezVisualScriptNode_Abs::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
  case 0:
    return &m_Value;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Sign, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Sign>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Math")
  }
    EZ_END_ATTRIBUTES;
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN("Value", 0, ezVisualScriptDataPinType::Number),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualScriptNode_Sign::ezVisualScriptNode_Sign() { }
ezVisualScriptNode_Sign::~ezVisualScriptNode_Sign() { }

void ezVisualScriptNode_Sign::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const double result = ezMath::Sign(m_Value);
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* ezVisualScriptNode_Sign::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
  case 0:
    return &m_Value;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////



EZ_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_VisualScriptMathNodes);

