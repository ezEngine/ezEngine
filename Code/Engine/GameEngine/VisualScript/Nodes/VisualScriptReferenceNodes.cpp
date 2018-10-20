#include <PCH.h>

#include <Core/World/GameObject.h>
#include <Core/World/World.h>
#include <GameEngine/VisualScript/Nodes/VisualScriptReferenceNodes.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_GetScriptOwner, 2, ezRTTIDefaultAllocator<ezVisualScriptNode_GetScriptOwner>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("References")
  }
    EZ_END_ATTRIBUTES;
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Object", 0, ezVisualScriptDataPinType::GameObjectHandle),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_GetScriptOwner::ezVisualScriptNode_GetScriptOwner() {}
ezVisualScriptNode_GetScriptOwner::~ezVisualScriptNode_GetScriptOwner() {}

void ezVisualScriptNode_GetScriptOwner::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  // we have no input values here that could change, but this will still be executed once after initial startup
  // after that the value will not change, so no need to re-execute
  if (m_bInputValuesChanged)
  {
    ezGameObjectHandle hObject = pInstance->GetOwner();
    pInstance->SetOutputPinValue(this, 0, &hObject);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_GetComponentOwner, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_GetComponentOwner>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("References")
  }
    EZ_END_ATTRIBUTES;
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN("Component", 0, ezVisualScriptDataPinType::ComponentHandle),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Object", 0, ezVisualScriptDataPinType::GameObjectHandle),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_GetComponentOwner::ezVisualScriptNode_GetComponentOwner() {}
ezVisualScriptNode_GetComponentOwner::~ezVisualScriptNode_GetComponentOwner() {}

void ezVisualScriptNode_GetComponentOwner::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    ezComponent* pComponent = nullptr;
    ezGameObjectHandle hObject;

    if (pInstance->GetWorld()->TryGetComponent(m_hComponent, pComponent))
    {
      hObject = pComponent->GetOwner()->GetHandle();
    }

    pInstance->SetOutputPinValue(this, 0, &hObject);
  }
}

void* ezVisualScriptNode_GetComponentOwner::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_hComponent;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_FindChildObject, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_FindChildObject>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("References"),
    new ezTitleAttribute("Child '{Name}'"),
  }
  EZ_END_ATTRIBUTES;
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN("Object", 0, ezVisualScriptDataPinType::GameObjectHandle),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Child", 0, ezVisualScriptDataPinType::GameObjectHandle),
    // Exposed Properties
    EZ_MEMBER_PROPERTY("Name", m_sChildObjectName),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_FindChildObject::ezVisualScriptNode_FindChildObject() {}
ezVisualScriptNode_FindChildObject::~ezVisualScriptNode_FindChildObject() {}

void ezVisualScriptNode_FindChildObject::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    if (m_hObject.IsInvalidated())
    {
      m_hObject = pInstance->GetOwner();
    }

    ezGameObject* pParent = nullptr;
    if (!pInstance->GetWorld()->TryGetObject(m_hObject, pParent))
    {
      ezLog::Warning("Script: FindChildObject: Cannot find child '{0}', parent object is already invalid.", m_sChildObjectName);
      return;
    }

    const ezTempHashedString name(m_sChildObjectName.GetData());
    ezGameObject* pChild = pParent->FindChildByName(name, true);

    if (pChild == nullptr)
    {
      ezLog::Warning("Script: Child-Object with Name '{0}' does not exist.", m_sChildObjectName);
      return;
    }

    ezGameObjectHandle hResult = pChild->GetHandle();
    pInstance->SetOutputPinValue(this, 0, &hResult);
  }
}


void* ezVisualScriptNode_FindChildObject::GetInputPinDataPointer(ezUInt8 uiPin)
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
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_FindComponent, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_FindComponent>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("References"),
    new ezTitleAttribute("Component '{Type}'"),
  }
  EZ_END_ATTRIBUTES;
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN("Object", 0, ezVisualScriptDataPinType::GameObjectHandle),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Component", 0, ezVisualScriptDataPinType::ComponentHandle),
    // Exposed Properties
    EZ_MEMBER_PROPERTY("Type", m_sType)->AddAttributes(new ezDynamicStringEnumAttribute("ComponentTypes")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_FindComponent::ezVisualScriptNode_FindComponent() {}
ezVisualScriptNode_FindComponent::~ezVisualScriptNode_FindComponent() {}

void ezVisualScriptNode_FindComponent::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    if (m_hObject.IsInvalidated())
    {
      m_hObject = pInstance->GetOwner();
    }

    ezGameObject* pParent = nullptr;
    if (!pInstance->GetWorld()->TryGetObject(m_hObject, pParent))
    {
      ezLog::Warning("Script: FindComponent: Cannot find component '{0}', parent object is already invalid.", m_sType);
      return;
    }

    const ezRTTI* pRtti = ezRTTI::FindTypeByName(m_sType);
    if (pRtti == nullptr)
    {
      ezLog::Error("Script: FindComponent: Component type '{0}' is unknown.", m_sType);
      return;
    }

    ezComponent* pComponent = nullptr;
    if (!pParent->TryGetComponentOfBaseType(pRtti, pComponent))
    {
      ezLog::Warning("Script: Component of type '{0}' does not exist at this node.", m_sType);
      return;
    }

    ezComponentHandle hResult = pComponent->GetHandle();
    pInstance->SetOutputPinValue(this, 0, &hResult);
  }
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

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_QueryGlobalObject, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_QueryGlobalObject>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("References"),
    new ezTitleAttribute("Global Object '{Name}'"),
  }
  EZ_END_ATTRIBUTES;
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Object", 0, ezVisualScriptDataPinType::GameObjectHandle),
    // Exposed Properties
    EZ_MEMBER_PROPERTY("Name", m_sObjectName),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_QueryGlobalObject::ezVisualScriptNode_QueryGlobalObject() {}
ezVisualScriptNode_QueryGlobalObject::~ezVisualScriptNode_QueryGlobalObject() {}

void ezVisualScriptNode_QueryGlobalObject::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const ezTempHashedString name(m_sObjectName.GetData());
    ezGameObject* pObject;

    if (!pInstance->GetWorld()->TryGetObjectWithGlobalKey(name, pObject))
    {
      ezLog::Warning("Script: Object with Global Key '{0}' does not exist.", m_sObjectName);
      return;
    }

    ezGameObjectHandle hResult = pObject->GetHandle();
    pInstance->SetOutputPinValue(this, 0, &hResult);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptNode_FindParent, 1, ezRTTIDefaultAllocator<ezVisualScriptNode_FindParent>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("References"),
    new ezTitleAttribute("Parent '{Name}'"),
  }
    EZ_END_ATTRIBUTES;
    EZ_BEGIN_PROPERTIES
  {
    // Data Pins (Input)
    EZ_INPUT_DATA_PIN("Object", 0, ezVisualScriptDataPinType::GameObjectHandle),
    // Exposed Properties
    EZ_MEMBER_PROPERTY("Name", m_sObjectName),
    // Data Pins (Output)
    EZ_OUTPUT_DATA_PIN("Parent", 0, ezVisualScriptDataPinType::GameObjectHandle),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptNode_FindParent::ezVisualScriptNode_FindParent() {}
ezVisualScriptNode_FindParent::~ezVisualScriptNode_FindParent() {}

void ezVisualScriptNode_FindParent::Execute(ezVisualScriptInstance* pInstance, ezUInt8 uiExecPin)
{
  if (m_bInputValuesChanged)
  {
    const ezTempHashedString name(m_sObjectName.GetData());
    ezGameObject* pObject;

    // 'self' if nothing is connected
    if (m_hObject.IsInvalidated())
    {
      m_hObject = pInstance->GetOwner();
    }

    if (pInstance->GetWorld()->TryGetObject(m_hObject, pObject))
    {
      // skip starting object
      pObject = pObject->GetParent();

      // search for a parent with the given name
      while (pObject != nullptr)
      {
        if (m_sObjectName.IsEmpty() || pObject->HasName(name))
        {
          ezGameObjectHandle hResult = pObject->GetHandle();
          pInstance->SetOutputPinValue(this, 0, &hResult);
          return;
        }

        pObject = pObject->GetParent();
      }
    }

    ezLog::Warning("Script: Parent Object with Name '{0}' could not be found.", m_sObjectName);
  }
}

void* ezVisualScriptNode_FindParent::GetInputPinDataPointer(ezUInt8 uiPin)
{
  switch (uiPin)
  {
    case 0:
      return &m_hObject;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////



EZ_STATICLINK_FILE(GameEngine, GameEngine_VisualScript_Nodes_VisualScriptReferenceNodes);
