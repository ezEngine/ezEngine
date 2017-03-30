#include <PCH.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptVariableNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Number, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Number>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Variables")
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sVariable),
    EZ_MEMBER_PROPERTY("Default", m_Value),
    EZ_OUTPUT_DATA_PIN("Value", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Number::ezVisualScriptNode_Number() { }
ezVisualScriptNode_Number::~ezVisualScriptNode_Number() { }

void ezVisualScriptNode_Number::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_VarName.GetHash() == 0)
  {
    m_VarName = m_sVariable.GetData();
  }

  pInstance->GetLocalVariables().RetrieveDouble(m_VarName, m_Value, m_Value);
  pInstance->SetOutputPinValue(this, 0, &m_Value);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_StoreNumber, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_StoreNumber>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Variables")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sVariable),
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value", 0, ezVisualScriptDataPinType::Number, m_Value),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_StoreNumber::ezVisualScriptNode_StoreNumber() { }
ezVisualScriptNode_StoreNumber::~ezVisualScriptNode_StoreNumber() { }

void ezVisualScriptNode_StoreNumber::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_VarName.GetHash() == 0)
  {
    m_VarName = m_sVariable.GetData();
  }

  pInstance->GetLocalVariables().StoreDouble(m_VarName, m_Value);
  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_StoreNumber::GetInputPinDataPointer(ezUInt8 uiPin)
{
  return &m_Value;
}

//////////////////////////////////////////////////////////////////////////


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Bool, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Bool>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Variables")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sVariable),
    EZ_MEMBER_PROPERTY("Default", m_Value),
    EZ_OUTPUT_DATA_PIN("Value", 0, ezVisualScriptDataPinType::Boolean),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_Bool::ezVisualScriptNode_Bool() { }
ezVisualScriptNode_Bool::~ezVisualScriptNode_Bool() { }

void ezVisualScriptNode_Bool::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_VarName.GetHash() == 0)
  {
    m_VarName = m_sVariable.GetData();
  }

  pInstance->GetLocalVariables().RetrieveBool(m_VarName, m_Value, m_Value);
  pInstance->SetOutputPinValue(this, 0, &m_Value);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_StoreBool, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_StoreBool>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Variables")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sVariable),
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value", 0, ezVisualScriptDataPinType::Boolean, m_Value),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_StoreBool::ezVisualScriptNode_StoreBool() { }
ezVisualScriptNode_StoreBool::~ezVisualScriptNode_StoreBool() { }

void ezVisualScriptNode_StoreBool::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_VarName.GetHash() == 0)
  {
    m_VarName = m_sVariable.GetData();
  }

  pInstance->GetLocalVariables().StoreBool(m_VarName, m_Value);
  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_StoreBool::GetInputPinDataPointer(ezUInt8 uiPin)
{
  return &m_Value;
}

//////////////////////////////////////////////////////////////////////////
