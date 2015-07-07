#include <Foundation/PCH.h>
#include <Foundation/Reflection/ReflectionAdapter.h>
#include <Foundation/Reflection/ReflectionUtils.h>

////////////////////////////////////////////////////////////////////////
// ezRttiSerializationContext
////////////////////////////////////////////////////////////////////////

void* ezRttiSerializationContext::CreateObject(const ezUuid& guid, const ezRTTI* pRtti)
{
  ezReflectedObjectWrapper* pWrapper = GetObjectByGUID(guid);
  if (pWrapper != nullptr)
  {
    return pWrapper->m_pObject;
  }

  if (!pRtti->GetAllocator()->CanAllocate())
    return nullptr;


  ezReflectedObjectWrapper object;
  object.m_pObject = pRtti->GetAllocator()->Allocate();
  object.m_pType = pRtti;
  m_Objects.Insert(guid, object);
  m_PtrLookup.Insert(object.m_pObject, guid);

  return object.m_pObject;
}

void ezRttiSerializationContext::DeleteObject(const ezUuid& guid)
{
  ezReflectedObjectWrapper* pObjectWrapper = nullptr;
  if (!m_Objects.TryGetValue(guid, pObjectWrapper))
  {
    EZ_REPORT_FAILURE("Given object guid can't be found and thus can't be deleted!");
    return;
  }

  if (!pObjectWrapper->m_pType->GetAllocator()->CanAllocate())
  {
    EZ_REPORT_FAILURE("Given object has no allocator and thus can't be deleted!");
    return;
  }

  const ezRTTI* pRtti = pObjectWrapper->m_pType;
  void* pObject = pObjectWrapper->m_pObject;
  m_PtrLookup.Remove(pObject);
  m_Objects.Remove(guid);
  m_PendingObjects.Remove(guid);
  pRtti->GetAllocator()->Deallocate(pObject);
}

void ezRttiSerializationContext::RegisterObject(const ezUuid& guid, const ezRTTI* pRtti, void* pObject)
{
  ezReflectedObjectWrapper object;
  object.m_pObject = pObject;
  object.m_pType = pRtti;
  m_Objects.Insert(guid, object);
  m_PtrLookup.Insert(object.m_pObject, guid);
}

ezReflectedObjectWrapper* ezRttiSerializationContext::GetObjectByGUID(const ezUuid& guid) const
{
  ezReflectedObjectWrapper* pObjectWrapper = nullptr;
  if (m_Objects.TryGetValue(guid, pObjectWrapper))
  {
    return pObjectWrapper;
  }
  return nullptr;
}

ezUuid ezRttiSerializationContext::GetObjectGUID(void* pObject) const
{
  ezUuid* pGuid = nullptr;
  if (m_PtrLookup.TryGetValue(pObject, pGuid))
  {
    return *pGuid;
  }
  return ezUuid();
}

ezUuid ezRttiSerializationContext::EnqueObject(void* pObject, const ezRTTI* pRtti)
{
  EZ_ASSERT_DEBUG(pObject != nullptr && pRtti != nullptr, "To-be-serialized object and ptr must be known!");
  ezUuid guid = GetObjectGUID(pObject);
  if (!guid.IsValid())
  {
    ezReflectedObjectWrapper object;
    object.m_pObject = pObject;
    object.m_pType = pRtti;
    guid.CreateNewUuid();
    m_Objects.Insert(guid, object);
    m_PtrLookup.Insert(object.m_pObject, guid);
    m_PendingObjects.Insert(guid);
  }
  return guid;
}

ezReflectedObjectWrapper ezRttiSerializationContext::DequeueObject()
{
  ezReflectedObjectWrapper object;
  object.m_pObject = nullptr;
  object.m_pType = nullptr;
  if (m_PendingObjects.IsEmpty())
  {
    return object;
  }
  ezUuid guid = m_PendingObjects.GetIterator().Key();
  m_PendingObjects.Remove(m_PendingObjects.GetIterator());
  if (!m_Objects.TryGetValue(guid, object))
  {
    EZ_REPORT_FAILURE("Failed to retrieve object via guid!");
  }
  return object;
}


////////////////////////////////////////////////////////////////////////
// ezRttiAdapter
////////////////////////////////////////////////////////////////////////

ezRttiAdapter::ezRttiAdapter(ezReflectedSerializationContext* pContext) : ezReflectionAdapter(pContext)
{
}

// Type Info
ezReflectedTypeWrapper ezRttiAdapter::GetTypeInfo(const ezRTTI* pRtti) const
{
  ezReflectedTypeWrapper info;

  info.m_Flags = pRtti->GetTypeFlags();
  info.m_pType = pRtti;
  info.m_szName = pRtti->GetTypeName();
  return info;
}

const ezRTTI* ezRttiAdapter::FindTypeByName(const char* szTypeName) const
{
  return ezRTTI::FindTypeByName(szTypeName);
}

const ezRTTI* ezRttiAdapter::GetParentType(const ezRTTI* pRtti) const
{
  return pRtti->GetParentType();
}

bool ezRttiAdapter::IsStandardType(const ezRTTI* pRtti) const
{
  return ezReflectionUtils::IsBasicType(pRtti) || pRtti == ezGetStaticRTTI<ezVariant>();
}

// Property Info
ezReflectedPropertyWrapper ezRttiAdapter::GetPropertyInfo(void* pProp) const
{
  ezReflectedPropertyWrapper info;

  ezAbstractProperty* pProp2 = static_cast<ezAbstractProperty*>(pProp);
  info.m_Category = pProp2->GetCategory();
  info.m_pProperty = pProp2;
  info.m_pType = pProp2->GetSpecificType();
  info.m_szName = pProp2->GetPropertyName();
  info.m_Flags = pProp2->GetFlags();

  EZ_ASSERT_DEBUG(info.m_pType != nullptr, "Not implemented!");
  
  return info;
}

ezUInt32 ezRttiAdapter::GetPropertyCount(const ezRTTI* pRtti) const
{
  return pRtti->GetProperties().GetCount();
}

void* ezRttiAdapter::GetProperty(const ezRTTI* pRtti, ezUInt32 uiIndex) const
{
  return pRtti->GetProperties()[uiIndex];
}

void* ezRttiAdapter::FindPropertyByName(const ezRTTI* pRtti, const char* szPropName) const
{
  return pRtti->FindPropertyByName(szPropName, true);
}

// Property Access
ezVariant ezRttiAdapter::GetPropertyValue(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index) const
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Constant:
    {
      ezAbstractConstantProperty* pProp3 = static_cast<ezAbstractConstantProperty*>(pProp);
      return pProp3->GetConstant();
    }
    break;
  case ezPropertyCategory::Member:
    {
      ezAbstractMemberProperty* pProp3 = static_cast<ezAbstractMemberProperty*>(pProp);
      return ezReflectionUtils::GetMemberPropertyValue(pProp3, object.m_pObject);
    }
    break;
  case ezPropertyCategory::Array:
    {
      ezAbstractArrayProperty* pProp3 = static_cast<ezAbstractArrayProperty*>(pProp);
      return ezReflectionUtils::GetArrayPropertyValue(pProp3, object.m_pObject, index.ConvertTo<ezUInt32>());
    }
    break;
  default:
    EZ_ASSERT_DEBUG(false, "Not implemented!");
    break;
  }
  return ezVariant();
}

void ezRttiAdapter::SetPropertyValue(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, const ezVariant& value)
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      ezAbstractMemberProperty* pProp3 = static_cast<ezAbstractMemberProperty*>(pProp);
      ezReflectionUtils::SetMemberPropertyValue(pProp3, object.m_pObject, value);
    }
    break;
  case ezPropertyCategory::Array:
    {
      ezAbstractArrayProperty* pProp3 = static_cast<ezAbstractArrayProperty*>(pProp);
      ezReflectionUtils::SetArrayPropertyValue(pProp3, object.m_pObject, index.ConvertTo<ezUInt32>(), value);
    }
    break;
  default:
    EZ_ASSERT_DEBUG(false, "Not implemented!");
    break;
  }
}

void ezRttiAdapter::GetPropertyObject(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, ezReflectedObjectWrapper& value) const
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      ezAbstractMemberProperty* pProp3 = static_cast<ezAbstractMemberProperty*>(pProp);
      pProp3->GetValuePtr(object.m_pObject, value.m_pObject);
    }
    break;
  case ezPropertyCategory::Array:
    {
      ezAbstractArrayProperty* pProp3 = static_cast<ezAbstractArrayProperty*>(pProp);
      pProp3->GetValue(object.m_pObject, index.ConvertTo<ezUInt32>(), value.m_pObject);
    }
    break;
  default:
    EZ_ASSERT_DEBUG(false, "Not implemented!");
    break;
  }
}

void ezRttiAdapter::SetPropertyObject(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, const ezReflectedObjectWrapper& value)
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      ezAbstractMemberProperty* pProp3 = static_cast<ezAbstractMemberProperty*>(pProp);
      pProp3->SetValuePtr(object.m_pObject, value.m_pObject);
    }
    break;
  case ezPropertyCategory::Array:
    {
      ezAbstractArrayProperty* pProp3 = static_cast<ezAbstractArrayProperty*>(pProp);
      pProp3->SetValue(object.m_pObject, index.ConvertTo<ezUInt32>(), value.m_pObject);
    }
    break;
  default:
    EZ_ASSERT_DEBUG(false, "Not implemented!");
    break;
  }
}

bool ezRttiAdapter::CanGetDirectPropertyPointer(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index) const
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  if (pProp->GetCategory() != ezPropertyCategory::Member)
    return false;

  ezAbstractMemberProperty* pProp2 = static_cast<ezAbstractMemberProperty*>(pProp);
  return pProp2->GetPropertyPointer(object.m_pObject) != nullptr;
}

ezReflectedObjectWrapper ezRttiAdapter::GetDirectPropertyPointer(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index)
{
  ezReflectedObjectWrapper subObject;
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  EZ_ASSERT_DEBUG(pProp->GetCategory() == ezPropertyCategory::Member, "");

  ezAbstractMemberProperty* pProp2 = static_cast<ezAbstractMemberProperty*>(pProp);

  subObject.m_pObject = pProp2->GetPropertyPointer(object.m_pObject);
  subObject.m_pType = prop.m_pType;
  return subObject;
}

// Array
ezUInt32 ezRttiAdapter::GetArrayElementCount(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop) const
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  EZ_ASSERT_DEBUG(pProp->GetCategory() == ezPropertyCategory::Array, "GetArrayElementCount can only be called for array properties!");

  ezAbstractArrayProperty* pProp3 = static_cast<ezAbstractArrayProperty*>(pProp);
  return pProp3->GetCount(object.m_pObject);
}

void ezRttiAdapter::SetArrayElementCount(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, ezUInt32 uiCount)
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  EZ_ASSERT_DEBUG(pProp->GetCategory() == ezPropertyCategory::Array, "SetArrayElementCount can only be called for array properties!");

  ezAbstractArrayProperty* pProp3 = static_cast<ezAbstractArrayProperty*>(pProp);
  pProp3->SetCount(object.m_pObject, uiCount);
}

// Set
void ezRttiAdapter::GetSetContent(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, ezHybridArray<ezVariant, 16>& out_Keys) const
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  EZ_ASSERT_DEBUG(pProp->GetCategory() == ezPropertyCategory::Set, "GetSetContent can only be called for set properties!");

  ezAbstractSetProperty* pProp3 = static_cast<ezAbstractSetProperty*>(pProp);
  pProp3->GetValues(object.m_pObject, out_Keys);
}

void ezRttiAdapter::SetSetContent(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezHybridArray<ezVariant, 16>& keys)
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  EZ_ASSERT_DEBUG(pProp->GetCategory() == ezPropertyCategory::Set, "GetSetContent can only be called for set properties!");

  ezAbstractSetProperty* pProp3 = static_cast<ezAbstractSetProperty*>(pProp);
  pProp3->Clear(object.m_pObject);

  for (const ezVariant& var : keys)
  {
    ezReflectionUtils::InsertSetPropertyValue(pProp3, object.m_pObject, var);
  }
}

// Allocate
bool ezRttiAdapter::CanCreateObject(const ezRTTI* pRtti)
{
  return pRtti->GetAllocator()->CanAllocate();
}
ezReflectedObjectWrapper ezRttiAdapter::CreateObject(const ezRTTI* pRtti)
{
  ezReflectedObjectWrapper object;

  object.m_pObject = pRtti->GetAllocator()->Allocate();
  object.m_pType = pRtti;

  return object;
}
void ezRttiAdapter::DeleteObject(ezReflectedObjectWrapper& object)
{
  object.m_pType->GetAllocator()->Deallocate(object.m_pObject);
  object.m_pObject = nullptr;
}

