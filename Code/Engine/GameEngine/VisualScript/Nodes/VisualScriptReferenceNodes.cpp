#include <PCH.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptReferenceNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_GetOwner, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_GetOwner>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("References")
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Object", 0, ezVisualScriptDataPinType::GameObjectHandle),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_GetOwner::ezVisualScriptNode_GetOwner() {}
ezVisualScriptNode_GetOwner::~ezVisualScriptNode_GetOwner() {}

void ezVisualScriptNode_GetOwner::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  ezGameObjectHandle hObject = pInstance->GetOwner()->GetHandle();
  pInstance->SetOutputPinValue(this, 0, &hObject);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_FindChildObject, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_FindChildObject>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("References"),
    new ezTitleAttribute("Child '{0}'"),
  }
  EZ_END_ATTRIBUTES
  EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Object", 0, ezVisualScriptDataPinType::GameObjectHandle),
    // Exposed Properties
    EZ_MEMBER_PROPERTY("Name", m_sObjectName),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_FindChildObject::ezVisualScriptNode_FindChildObject() {}
ezVisualScriptNode_FindChildObject::~ezVisualScriptNode_FindChildObject() {}

void ezVisualScriptNode_FindChildObject::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_hObject.IsInvalidated() && !m_sObjectName.IsEmpty())
  {
    const ezTempHashedString name(m_sObjectName.GetData());
    ezGameObject* pChild = pInstance->GetOwner()->FindChildByName(name, true);

    if (pChild != nullptr)
    {
      m_hObject = pChild->GetHandle();
    }
    else
    {
      // make sure we don't try this again
      ezLog::Warning("Script: Child-Object with Name '{0}' does not exist.", m_sObjectName);
      m_sObjectName.Clear();
      m_hObject.Invalidate();
    }
  }

  pInstance->SetOutputPinValue(this, 0, &m_hObject);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_FindComponent, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_FindComponent>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("References"),
    new ezTitleAttribute("Component '{0}'"),
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN("Object", 0, ezVisualScriptDataPinType::GameObjectHandle),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Component", 0, ezVisualScriptDataPinType::ComponentHandle),
    // Exposed Properties
    EZ_MEMBER_PROPERTY("Type", m_sType)->AddAttributes(new ezDynamicStringEnumAttribute("ComponentTypes")),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_FindComponent::ezVisualScriptNode_FindComponent() {}
ezVisualScriptNode_FindComponent::~ezVisualScriptNode_FindComponent() {}

void ezVisualScriptNode_FindComponent::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_hComponent.IsInvalidated() && !m_sType.IsEmpty())
  {
    ezGameObject* pObject = pInstance->GetOwner();

    if (!m_hObject.IsInvalidated())
    {
      if (!pObject->GetWorld()->TryGetObject(m_hObject, pObject))
        goto fail;
    }

    const ezRTTI* pRtti = ezRTTI::FindTypeByName(m_sType);
    if (pRtti == nullptr)
      goto fail;

    ezComponent* pComponent = nullptr;
    if (!pObject->TryGetComponentOfBaseType(pRtti, pComponent))
      goto fail;

    m_hComponent = pComponent->GetHandle();
  }

  pInstance->SetOutputPinValue(this, 0, &m_hComponent);
  return;

fail:
  ezLog::Warning("Script: Component of type '{0}' does not exist at this node.", m_sType);
  m_hComponent.Invalidate();
  m_sType.Clear();
}

void* ezVisualScriptNode_FindComponent::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
  case 0:
    return &m_hObject;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_QueryGlobalObject, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_QueryGlobalObject>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("References"),
    new ezTitleAttribute("Global Object '{0}'"),
  }
    EZ_END_ATTRIBUTES
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Object", 0, ezVisualScriptDataPinType::GameObjectHandle),
    // Exposed Properties
    EZ_MEMBER_PROPERTY("Name", m_sObjectName),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptNode_QueryGlobalObject::ezVisualScriptNode_QueryGlobalObject() {}
ezVisualScriptNode_QueryGlobalObject::~ezVisualScriptNode_QueryGlobalObject() {}

void ezVisualScriptNode_QueryGlobalObject::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_hObject.IsInvalidated() && !m_sObjectName.IsEmpty())
  {
    const ezTempHashedString name(m_sObjectName.GetData());
    ezGameObject* pObject;

    if (pInstance->GetWorld()->TryGetObjectWithGlobalKey(name, pObject))
    {
      m_hObject = pObject->GetHandle();
    }
    else
    {
      // make sure we don't try this again
      ezLog::Warning("Script: Object with Global Key '{0}' does not exist.", m_sObjectName);
      m_sObjectName.Clear();
      m_hObject.Invalidate();
    }
  }

  pInstance->SetOutputPinValue(this, 0, &m_hObject);
}

//////////////////////////////////////////////////////////////////////////


