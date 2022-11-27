#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/VisualScript/Nodes/VisualScriptMathExpressionNode.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_MathExpression, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_MathExpression>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Math"),
    new ezTitleAttribute("Expression '{Expression}'"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN_AND_PROPERTY("a", 0, ezVisualScriptDataPinType::Number, m_ValueA)->AddAttributes(new ezDefaultValueAttribute(0.0)),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("b", 1, ezVisualScriptDataPinType::Number, m_ValueB)->AddAttributes(new ezDefaultValueAttribute(1.0)),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("c", 2, ezVisualScriptDataPinType::Number, m_ValueC)->AddAttributes(new ezDefaultValueAttribute(2.0)),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("d", 3, ezVisualScriptDataPinType::Number, m_ValueD)->AddAttributes(new ezDefaultValueAttribute(3.0)),
    EZ_ACCESSOR_PROPERTY("Expression", GetExpression, SetExpression),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_MathExpression::ezVisualScriptNode_MathExpression() {}
ezVisualScriptNode_MathExpression::~ezVisualScriptNode_MathExpression() {}

static ezHashedString s_sA = ezMakeHashedString("a");
static ezHashedString s_sB = ezMakeHashedString("b");
static ezHashedString s_sC = ezMakeHashedString("c");
static ezHashedString s_sD = ezMakeHashedString("d");

void ezVisualScriptNode_MathExpression::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    ezMathExpression::Input inputs[] =
      {
        {s_sA, static_cast<float>(m_ValueA)},
        {s_sB, static_cast<float>(m_ValueB)},
        {s_sC, static_cast<float>(m_ValueC)},
        {s_sD, static_cast<float>(m_ValueD)},
      };

    const double result = m_mMathExpression.Evaluate(inputs);
    pInstance->SetOutputPinValue(this, 0, &result);
  }
}

void* ezVisualScriptNode_MathExpression::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_ValueA;
    case 1:
      return &m_ValueB;
    case 2:
      return &m_ValueC;
    case 3:
      return &m_ValueD;
  }

  return nullptr;
}

const char* ezVisualScriptNode_MathExpression::GetExpression() const
{
  return m_mMathExpression.GetExpressionString();
}

void ezVisualScriptNode_MathExpression::SetExpression(const char* e)
{
  m_mMathExpression.Reset(e);
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_VisualScriptMathExpressionNode);
