#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptVariableNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_GetNumberProperty, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_GetNumberProperty>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Properties"),
    new ezTitleAttribute("Get Number Property '{Name}'"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    EZ_INPUT_DATA_PIN("Component", 0, ezVisualScriptDataPinType::ComponentHandle),
    EZ_OUTPUT_DATA_PIN("Value", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_GetNumberProperty::ezVisualScriptNode_GetNumberProperty() {}
ezVisualScriptNode_GetNumberProperty::~ezVisualScriptNode_GetNumberProperty() {}

void ezVisualScriptNode_GetNumberProperty::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  double value = 0;

  ezComponent* pComponent = nullptr;
  if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
  {
    ezAbstractProperty* pAbsProp = pComponent->GetDynamicRTTI()->FindPropertyByName(m_sVariable);

    if (pAbsProp && pAbsProp->GetCategory() == ezPropertyCategory::Member)
    {
      ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbsProp);

      ezVariant var = ezReflectionUtils::GetMemberPropertyValue(pMember, pComponent);

      if (var.CanConvertTo<double>())
      {
        value = var.ConvertTo<double>();
        pInstance->SetOutputPinValue(this, 0, &value);
        return;
      }
    }
  }

  ezLog::Warning("Script: Number Property '{0}' could not be found on the given component.", m_sVariable);
}


void* ezVisualScriptNode_GetNumberProperty::GetInputPinDataPointer(ezUInt8 uiPin)
{
  return &m_hComponent;
}


//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_SetNumberProperty, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_SetNumberProperty>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Properties"),
    new ezTitleAttribute("Set Number Property '{Name}' = {Value}"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    EZ_INPUT_DATA_PIN("Component", 0, ezVisualScriptDataPinType::ComponentHandle),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value", 1, ezVisualScriptDataPinType::Number, m_fValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_SetNumberProperty::ezVisualScriptNode_SetNumberProperty() {}
ezVisualScriptNode_SetNumberProperty::~ezVisualScriptNode_SetNumberProperty() {}

void ezVisualScriptNode_SetNumberProperty::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  ezComponent* pComponent = nullptr;
  if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
  {
    ezAbstractProperty* pAbsProp = pComponent->GetDynamicRTTI()->FindPropertyByName(m_sVariable);

    if (pAbsProp && pAbsProp->GetCategory() == ezPropertyCategory::Member)
    {
      ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbsProp);

      ezReflectionUtils::SetMemberPropertyValue(pMember, pComponent, m_fValue);
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_SetNumberProperty::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_hComponent;
    case 1:
      return &m_fValue;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_GetBoolProperty, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_GetBoolProperty>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Properties"),
    new ezTitleAttribute("Get Bool Property '{Name}'"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    EZ_INPUT_DATA_PIN("Component", 0, ezVisualScriptDataPinType::ComponentHandle),
    EZ_OUTPUT_DATA_PIN("Value", 0, ezVisualScriptDataPinType::Boolean),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_GetBoolProperty::ezVisualScriptNode_GetBoolProperty() {}
ezVisualScriptNode_GetBoolProperty::~ezVisualScriptNode_GetBoolProperty() {}

void ezVisualScriptNode_GetBoolProperty::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  bool value = 0;

  ezComponent* pComponent = nullptr;
  if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
  {
    ezAbstractProperty* pAbsProp = pComponent->GetDynamicRTTI()->FindPropertyByName(m_sVariable);

    if (pAbsProp && pAbsProp->GetCategory() == ezPropertyCategory::Member)
    {
      ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbsProp);

      ezVariant var = ezReflectionUtils::GetMemberPropertyValue(pMember, pComponent);

      if (var.CanConvertTo<bool>())
      {
        value = var.ConvertTo<bool>();
        pInstance->SetOutputPinValue(this, 0, &value);
        return;
      }
    }
  }

  ezLog::Warning("Script: Bool Property '{0}' could not be found on the given component.", m_sVariable);
}


void* ezVisualScriptNode_GetBoolProperty::GetInputPinDataPointer(ezUInt8 uiPin)
{
  return &m_hComponent;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_SetBoolProperty, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_SetBoolProperty>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Properties"),
    new ezTitleAttribute("Set Bool Property '{Name}' = {Value}"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    EZ_INPUT_DATA_PIN("Component", 0, ezVisualScriptDataPinType::ComponentHandle),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value", 1, ezVisualScriptDataPinType::Boolean, m_bValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_SetBoolProperty::ezVisualScriptNode_SetBoolProperty() {}
ezVisualScriptNode_SetBoolProperty::~ezVisualScriptNode_SetBoolProperty() {}

void ezVisualScriptNode_SetBoolProperty::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  ezComponent* pComponent = nullptr;
  if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
  {
    ezAbstractProperty* pAbsProp = pComponent->GetDynamicRTTI()->FindPropertyByName(m_sVariable);

    if (pAbsProp && pAbsProp->GetCategory() == ezPropertyCategory::Member)
    {
      ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbsProp);

      ezReflectionUtils::SetMemberPropertyValue(pMember, pComponent, m_bValue);
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_SetBoolProperty::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_hComponent;
    case 1:
      return &m_bValue;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_GetStringProperty, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_GetStringProperty>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Properties"),
    new ezTitleAttribute("Get String Property '{Name}'"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    EZ_INPUT_DATA_PIN("Component", 0, ezVisualScriptDataPinType::ComponentHandle),
    EZ_OUTPUT_DATA_PIN("Value", 0, ezVisualScriptDataPinType::String),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_GetStringProperty::ezVisualScriptNode_GetStringProperty() {}
ezVisualScriptNode_GetStringProperty::~ezVisualScriptNode_GetStringProperty() {}

void ezVisualScriptNode_GetStringProperty::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  ezString value;

  ezComponent* pComponent = nullptr;
  if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
  {
    ezAbstractProperty* pAbsProp = pComponent->GetDynamicRTTI()->FindPropertyByName(m_sVariable);

    if (pAbsProp && pAbsProp->GetCategory() == ezPropertyCategory::Member)
    {
      ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbsProp);

      ezVariant var = ezReflectionUtils::GetMemberPropertyValue(pMember, pComponent);

      if (var.CanConvertTo<ezString>())
      {
        value = var.ConvertTo<ezString>();
        pInstance->SetOutputPinValue(this, 0, &value);
        return;
      }
    }
  }

  ezLog::Warning("Script: String Property '{0}' could not be found on the given component.", m_sVariable);
}


void* ezVisualScriptNode_GetStringProperty::GetInputPinDataPointer(ezUInt8 uiPin)
{
  return &m_hComponent;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_SetStringProperty, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_SetStringProperty>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Properties"),
    new ezTitleAttribute("Set String Property '{Name}' = {Value}"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    EZ_INPUT_DATA_PIN("Component", 0, ezVisualScriptDataPinType::ComponentHandle),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value", 1, ezVisualScriptDataPinType::String, m_sValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_SetStringProperty::ezVisualScriptNode_SetStringProperty() {}
ezVisualScriptNode_SetStringProperty::~ezVisualScriptNode_SetStringProperty() {}

void ezVisualScriptNode_SetStringProperty::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  ezComponent* pComponent = nullptr;
  if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
  {
    ezAbstractProperty* pAbsProp = pComponent->GetDynamicRTTI()->FindPropertyByName(m_sVariable);

    if (pAbsProp && pAbsProp->GetCategory() == ezPropertyCategory::Member)
    {
      ezAbstractMemberProperty* pMember = static_cast<ezAbstractMemberProperty*>(pAbsProp);

      ezReflectionUtils::SetMemberPropertyValue(pMember, pComponent, m_sValue);
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_SetStringProperty::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_hComponent;
    case 1:
      return &m_sValue;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Number, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Number>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Variables"),
    new ezTitleAttribute("Number '{Name}'"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    EZ_OUTPUT_DATA_PIN("Value", 0, ezVisualScriptDataPinType::Number),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_Number::ezVisualScriptNode_Number() {}
ezVisualScriptNode_Number::~ezVisualScriptNode_Number() {}

void ezVisualScriptNode_Number::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  double fCurrentValue = 0.0;
  pInstance->GetLocalVariables().RetrieveDouble(m_sVariable, fCurrentValue, 0.0);
  pInstance->SetOutputPinValue(this, 0, &fCurrentValue);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_StoreNumber, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_StoreNumber>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Variables"),
    new ezTitleAttribute("Store Number '{Name}' = {Value}"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value", 0, ezVisualScriptDataPinType::Number, m_Value),
    EZ_OUTPUT_DATA_PIN("StoredValue", 0, ezVisualScriptDataPinType::Number)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_StoreNumber::ezVisualScriptNode_StoreNumber() {}
ezVisualScriptNode_StoreNumber::~ezVisualScriptNode_StoreNumber() {}

void ezVisualScriptNode_StoreNumber::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  pInstance->GetLocalVariables().StoreDouble(m_sVariable, m_Value);
  pInstance->SetOutputPinValue(this, 0, &m_Value);
  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_StoreNumber::GetInputPinDataPointer(ezUInt8 uiPin)
{
  return &m_Value;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_Bool, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_Bool>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Variables"),
    new ezTitleAttribute("Bool '{Name}'"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    EZ_OUTPUT_DATA_PIN("Value", 0, ezVisualScriptDataPinType::Boolean),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_Bool::ezVisualScriptNode_Bool() {}
ezVisualScriptNode_Bool::~ezVisualScriptNode_Bool() {}

void ezVisualScriptNode_Bool::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  bool bCurrentValue = false;
  pInstance->GetLocalVariables().RetrieveBool(m_sVariable, bCurrentValue, false);
  pInstance->SetOutputPinValue(this, 0, &bCurrentValue);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_StoreBool, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_StoreBool>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Variables"),
    new ezTitleAttribute("Store Bool '{Name}' = {Value}"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value", 0, ezVisualScriptDataPinType::Boolean, m_Value),
    EZ_OUTPUT_DATA_PIN("StoredValue", 0, ezVisualScriptDataPinType::Boolean)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_StoreBool::ezVisualScriptNode_StoreBool() {}
ezVisualScriptNode_StoreBool::~ezVisualScriptNode_StoreBool() {}

void ezVisualScriptNode_StoreBool::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  pInstance->GetLocalVariables().StoreBool(m_sVariable, m_Value);
  pInstance->SetOutputPinValue(this, 0, &m_Value);
  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_StoreBool::GetInputPinDataPointer(ezUInt8 uiPin)
{
  return &m_Value;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_ToggleBool, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_ToggleBool>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Variables"),
    new ezTitleAttribute("Toggle Bool '{Name}'"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("OnTrue", 0),
    EZ_OUTPUT_EXECUTION_PIN("OnFalse", 1),
    EZ_OUTPUT_DATA_PIN("Result", 0, ezVisualScriptDataPinType::Boolean),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_ToggleBool::ezVisualScriptNode_ToggleBool() {}
ezVisualScriptNode_ToggleBool::~ezVisualScriptNode_ToggleBool() {}

void ezVisualScriptNode_ToggleBool::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  bool bCurrentValue = false;
  pInstance->GetLocalVariables().RetrieveBool(m_sVariable, bCurrentValue, false);
  bCurrentValue = !bCurrentValue;

  pInstance->GetLocalVariables().StoreBool(m_sVariable, bCurrentValue);
  pInstance->SetOutputPinValue(this, 0, &bCurrentValue);

  pInstance->ExecuteConnectedNodes(this, bCurrentValue ? 0 : 1);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_String, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_String>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Variables"),
    new ezTitleAttribute("String '{Name}'"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    EZ_OUTPUT_DATA_PIN("Value", 0, ezVisualScriptDataPinType::String),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_String::ezVisualScriptNode_String() {}
ezVisualScriptNode_String::~ezVisualScriptNode_String() {}

void ezVisualScriptNode_String::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  ezString sCurrentValue;
  pInstance->GetLocalVariables().RetrieveString(m_sVariable, sCurrentValue, "");
  pInstance->SetOutputPinValue(this, 0, &sCurrentValue);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_StoreString, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_StoreString>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Variables"),
    new ezTitleAttribute("Store String '{Name}' = {Value}"),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Name", GetVariable, SetVariable),
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    EZ_INPUT_DATA_PIN_AND_PROPERTY("Value", 0, ezVisualScriptDataPinType::String, m_sValue),
    EZ_OUTPUT_DATA_PIN("StoredValue", 0, ezVisualScriptDataPinType::String)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_StoreString::ezVisualScriptNode_StoreString() {}
ezVisualScriptNode_StoreString::~ezVisualScriptNode_StoreString() {}

void ezVisualScriptNode_StoreString::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  pInstance->GetLocalVariables().StoreString(m_sVariable, m_sValue);
  pInstance->SetOutputPinValue(this, 0, &m_sValue);
  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_StoreString::GetInputPinDataPointer(ezUInt8 uiPin)
{
  return &m_sValue;
}

//////////////////////////////////////////////////////////////////////////



EZ_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_VisualScriptVariableNodes);
