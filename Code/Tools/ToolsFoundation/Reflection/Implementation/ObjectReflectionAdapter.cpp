#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Reflection/ObjectReflectionAdapter.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

////////////////////////////////////////////////////////////////////////
// ezObjectSerializationContext
////////////////////////////////////////////////////////////////////////

void* ezObjectSerializationContext::CreateObject(const ezUuid& guid, const ezRTTI* pType)
{
  ezReflectedObjectWrapper* pWrapper = GetObjectByGUID(guid);
  if (pWrapper != nullptr)
  {
    return pWrapper->m_pObject;
  }

  EZ_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
}

void ezObjectSerializationContext::DeleteObject(const ezUuid& guid)
{
  ezReflectedObjectWrapper* pObjectWrapper = nullptr;
  if (!m_Objects.TryGetValue(guid, pObjectWrapper))
  {
    EZ_REPORT_FAILURE("Given object guid can't be found and thus can't be deleted!");
    return;
  }

  const ezRTTI* pRtti = static_cast<const ezRTTI*>(pObjectWrapper->m_pType);

  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezObjectSerializationContext::RegisterObject(const ezUuid& guid, const ezRTTI* pType, void* pObject)
{
  ezReflectedObjectWrapper object;
  object.m_pObject = pObject;
  object.m_pType = pType;
  m_Objects.Insert(guid, object);
  m_PtrLookup.Insert(object.m_pObject, guid);
}

ezReflectedObjectWrapper* ezObjectSerializationContext::GetObjectByGUID(const ezUuid& guid) const
{
  ezReflectedObjectWrapper* pObjectWrapper = nullptr;
  if (m_Objects.TryGetValue(guid, pObjectWrapper))
  {
    return pObjectWrapper;
  }
  return nullptr;
}

ezUuid ezObjectSerializationContext::GetObjectGUID(void* pObject) const
{
  ezUuid* pGuid = nullptr;
  if (m_PtrLookup.TryGetValue(pObject, pGuid))
  {
    return *pGuid;
  }
  return ezUuid();
}

ezUuid ezObjectSerializationContext::EnqueObject(void* pObject, const ezRTTI* pType)
{
  ezUuid guid = GetObjectGUID(pObject);
  if (!guid.IsValid())
  {
    ezDocumentObjectBase* pDocObj = static_cast<ezDocumentObjectBase*>(pObject);
    guid = pDocObj->GetGuid();

    ezReflectedObjectWrapper object;
    object.m_pObject = pObject;
    object.m_pType = pType;

    m_Objects.Insert(guid, object);
    m_PtrLookup.Insert(object.m_pObject, guid);
    m_PendingObjects.Insert(guid);
  }
  return guid;
}

ezReflectedObjectWrapper ezObjectSerializationContext::DequeueObject()
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
// ezObjectReflectionAdapter
////////////////////////////////////////////////////////////////////////

ezObjectReflectionAdapter::ezObjectReflectionAdapter(ezObjectSerializationContext* pContext) : ezRttiAdapter(pContext)
{
}

// Property Access
ezVariant ezObjectReflectionAdapter::GetPropertyValue(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index) const
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
      ezDocumentObjectBase* pDocObj = static_cast<ezDocumentObjectBase*>(object.m_pObject);
      return pDocObj->GetTypeAccessor().GetValue(ezPropertyPath(pProp->GetPropertyName()));
    }
    break;
  case ezPropertyCategory::Array:
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
    break;
  case ezPropertyCategory::Set:
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
    break;
  default:
    EZ_ASSERT_DEBUG(false, "Not implemented!");
    break;
  }
  return ezVariant();
}

void ezObjectReflectionAdapter::SetPropertyValue(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, const ezVariant& value)
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      ezDocumentObjectBase* pDocObj = static_cast<ezDocumentObjectBase*>(object.m_pObject);
      pDocObj->GetTypeAccessor().SetValue(ezPropertyPath(pProp->GetPropertyName()), value);
    }
    break;
  case ezPropertyCategory::Array:
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
    break;
  case ezPropertyCategory::Set:
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
    break;
  default:
    EZ_ASSERT_DEBUG(false, "Not implemented!");
    break;
  }
}

void ezObjectReflectionAdapter::GetPropertyObject(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index, ezReflectedObjectWrapper& value) const
{
  ezAbstractProperty* pProp = static_cast<ezAbstractProperty*>(prop.m_pProperty);
  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      ezDocumentSubObject* pObject = static_cast<ezDocumentSubObject*>(value.m_pObject);
      pObject->SetObject(static_cast<ezDocumentObjectBase*>(object.m_pObject), ezPropertyPath(prop.m_szName));
    }
    break;
  case ezPropertyCategory::Array:
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
    break;
  case ezPropertyCategory::Set:
    {
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
    break;
  default:
    EZ_ASSERT_DEBUG(false, "Not implemented!");
    break;
  }
}

bool ezObjectReflectionAdapter::CanGetDirectPropertyPointer(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index) const
{
  return false;
}

ezReflectedObjectWrapper ezObjectReflectionAdapter::GetDirectPropertyPointer(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezVariant& index)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezReflectedObjectWrapper();
}

// Array
ezUInt32 ezObjectReflectionAdapter::GetArrayElementCount(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop) const
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

void ezObjectReflectionAdapter::SetArrayElementCount(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, ezUInt32 uiCount)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

// Set
void ezObjectReflectionAdapter::GetSetContent(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, ezHybridArray<ezVariant, 16>& out_Keys) const
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

void ezObjectReflectionAdapter::SetSetContent(const ezReflectedObjectWrapper& object, const ezReflectedPropertyWrapper& prop, const ezHybridArray<ezVariant, 16>& keys)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

// Allocate
bool ezObjectReflectionAdapter::CanCreateObject(const ezRTTI* pType)
{
  return true;
}

ezReflectedObjectWrapper ezObjectReflectionAdapter::CreateObject(const ezRTTI* pType)
{
  ezReflectedObjectWrapper object;

  const ezRTTI* pRtti = static_cast<const ezRTTI*>(pType);
  object.m_pObject = EZ_DEFAULT_NEW(ezDocumentSubObject, pRtti);
  object.m_pType = pRtti;

  return object;
}
void ezObjectReflectionAdapter::DeleteObject(ezReflectedObjectWrapper& object)
{
  ezDocumentSubObject* pObject = static_cast<ezDocumentSubObject*>(object.m_pObject);
  EZ_DEFAULT_DELETE(pObject);
}

