#include <PCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptObjectNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_DeleteObject, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_DeleteObject>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Objects")
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    // Execution Pins (Input)
    EZ_INPUT_EXECUTION_PIN("run", 0),
    // Execution Pins (Output)
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN("Object", 0, ezVisualScriptDataPinType::GameObjectHandle),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_DeleteObject::ezVisualScriptNode_DeleteObject() {}

void ezVisualScriptNode_DeleteObject::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (!m_hObject.IsInvalidated())
  {
    pInstance->GetWorld()->DeleteObjectDelayed(m_hObject);
  }
  else
  {
    pInstance->GetWorld()->DeleteObjectDelayed(pInstance->GetOwner());
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_DeleteObject::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_hObject;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_ActivateComponent, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_ActivateComponent>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Components")
  }
    EZ_END_ATTRIBUTES;
    EZ_BEGIN_PROPERTIES
  {
    // Execution Pins (Input)
    EZ_INPUT_EXECUTION_PIN("Activate", 0),
    EZ_INPUT_EXECUTION_PIN("Deactivate", 1),
    // Execution Pins (Output)
    EZ_OUTPUT_EXECUTION_PIN("then", 0),
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN("Component", 0, ezVisualScriptDataPinType::ComponentHandle),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_ActivateComponent::ezVisualScriptNode_ActivateComponent() {}

void ezVisualScriptNode_ActivateComponent::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (!m_hComponent.IsInvalidated())
  {
    ezComponent* pComponent = nullptr;
    if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
    {
      if (uiExecPin == 0)
      {
        pComponent->SetActive(true);
      }
      else
      {
        pComponent->SetActive(false);
      }
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_ActivateComponent::GetInputPinDataPointer(ezUInt8 uiPin)
{
  return &m_hComponent;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_HasName, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_HasName>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Objects"),
    new ezTitleAttribute("HasName '{ObjectName}'")
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectName", m_sObjectName),
    // Execution Pins
    EZ_INPUT_EXECUTION_PIN("run", 0),
    EZ_OUTPUT_EXECUTION_PIN("OnTrue", 0),
    EZ_OUTPUT_EXECUTION_PIN("OnFalse", 1),
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN("Object", 0, ezVisualScriptDataPinType::GameObjectHandle),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_HasName::ezVisualScriptNode_HasName() {}

void ezVisualScriptNode_HasName::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  ezGameObjectHandle hObject = m_hObject.IsInvalidated() ? pInstance->GetOwner() : m_hObject;

  ezGameObject* pObject = nullptr;
  if (pInstance->GetWorld()->TryGetObject(hObject, pObject))
  {
    if (m_sObjectName == pObject->GetName())
    {
      pInstance->ExecuteConnectedNodes(this, 0);
      return;
    }
  }

  pInstance->ExecuteConnectedNodes(this, 1);
}

void* ezVisualScriptNode_HasName::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
  case 0:
    return &m_hObject;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////



EZ_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_VisualScriptObjectNodes);

