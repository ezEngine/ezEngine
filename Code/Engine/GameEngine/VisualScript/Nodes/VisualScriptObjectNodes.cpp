#include <PCH.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptObjectNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_DeleteObject, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_DeleteObject>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Objects")
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_PROPERTIES
  {
    // Execution Pins (Input)
    EZ_CONSTANT_PROPERTY("run", 0)->AddAttributes(new ezVisScriptExecPinInAttribute(0)),
    // Execution Pins (Output)
    EZ_CONSTANT_PROPERTY("then", 0)->AddAttributes(new ezVisScriptExecPinOutAttribute(0)),
    // Data Pins (Input)
    EZ_CONSTANT_PROPERTY("Object", 0)->AddAttributes(new ezVisScriptDataPinInAttribute(0, ezVisualScriptDataPinType::GameObjectHandle)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_DeleteObject::ezVisualScriptNode_DeleteObject() { }

void ezVisualScriptNode_DeleteObject::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (!m_hObject.IsInvalidated())
  {
    pInstance->GetOwner()->GetWorld()->DeleteObjectDelayed(m_hObject);
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

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_ActivateComponent, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_ActivateComponent>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Components")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    // Execution Pins (Input)
    EZ_CONSTANT_PROPERTY("run", 0)->AddAttributes(new ezVisScriptExecPinInAttribute(0)),
    // Execution Pins (Output)
    EZ_CONSTANT_PROPERTY("then", 0)->AddAttributes(new ezVisScriptExecPinOutAttribute(0)),
    // Data Pins (Input)
    EZ_CONSTANT_PROPERTY("Component", 0)->AddAttributes(new ezVisScriptDataPinInAttribute(0, ezVisualScriptDataPinType::ComponentHandle)),
    EZ_CONSTANT_PROPERTY("Activate", 0)->AddAttributes(new ezVisScriptDataPinInAttribute(1, ezVisualScriptDataPinType::Boolean)),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_ActivateComponent::ezVisualScriptNode_ActivateComponent() { }

void ezVisualScriptNode_ActivateComponent::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (!m_hComponent.IsInvalidated())
  {
    ezComponent* pComponent = nullptr;
    if (pInstance->GetOwner()->GetWorld()->TryGetComponent(m_hComponent, pComponent))
    {
      pComponent->SetActive(m_bActive);
    }
  }

  pInstance->ExecuteConnectedNodes(this, 0);
}

void* ezVisualScriptNode_ActivateComponent::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
  case 0:
    return &m_hComponent;
  case 1:
    return &m_bActive;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

