#include <GameEnginePCH.h>

#include <GameEngine/VisualScript/Nodes/VisualScriptStringNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_IsEqual, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_IsEqual>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("String"),
    new ezTitleAttribute("IsEqual: {Value1} = {Value2}"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    // Properties
    EZ_MEMBER_PROPERTY("IgnoreCase", m_bIgnoreCase),

    // Data Pins (Input)
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value1", 0, ezVisualScriptDataPinType::String, m_sValue1),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value2", 1, ezVisualScriptDataPinType::String, m_sValue2),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::Boolean),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_IsEqual::ezVisualScriptNode_IsEqual() = default;
ezVisualScriptNode_IsEqual::~ezVisualScriptNode_IsEqual() = default;

void ezVisualScriptNode_IsEqual::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  // we can skip this and save some performance, if the inputs did not change, because the result won't have changed either
  if (m_bInputValuesChanged)
  {
    bool result;
    if (m_bIgnoreCase)
    {
      result = m_sValue1.IsEqual_NoCase(m_sValue2);
    }
    else
    {
      result = m_sValue1.IsEqual(m_sValue2);
    }

    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* ezVisualScriptNode_IsEqual::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_sValue1;

    case 1:
      return &m_sValue2;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Format, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Format>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("String"),
    new ezTitleAttribute("Format: '{Format}'"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    // Properties
    EZ_MEMBER_PROPERTY("Format", m_sFormat)->AddAttributes(new ezDefaultValueAttribute(ezStringView("Value1: {0}, Value2: {1}, Value3: {2}"))),
    // Data Pins
    EZ_MEMBER_PROPERTY("Value0", m_Value0)->AddAttributes(new ezVisScriptDataPinInAttribute(0, ezVisualScriptDataPinType::Variant), new ezDefaultValueAttribute(0)),
    EZ_MEMBER_PROPERTY("Value1", m_Value1)->AddAttributes(new ezVisScriptDataPinInAttribute(1, ezVisualScriptDataPinType::Variant), new ezDefaultValueAttribute(0)),
    EZ_MEMBER_PROPERTY("Value2", m_Value2)->AddAttributes(new ezVisScriptDataPinInAttribute(2, ezVisualScriptDataPinType::Variant), new ezDefaultValueAttribute(0)),
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::String)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_Format::ezVisualScriptNode_Format() {}
ezVisualScriptNode_Format::~ezVisualScriptNode_Format() {}

void ezVisualScriptNode_Format::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    ezStringBuilder sb;
    sb.Format(m_sFormat, m_Value0, m_Value1, m_Value2);

    ezString result = sb;
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* ezVisualScriptNode_Format::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_Value0;
    case 1:
      return &m_Value1;
    case 2:
      return &m_Value2;
  }

  return nullptr;
}
