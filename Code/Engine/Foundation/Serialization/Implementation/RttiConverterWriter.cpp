#include <PCH.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Reflection/ReflectionUtils.h>

void ezRttiConverterContext::Clear()
{
  m_GuidToObject.Clear();
  m_ObjectToGuid.Clear();
  m_QueuedObjects.Clear();
}

void* ezRttiConverterContext::CreateObject(const ezUuid& guid, const ezRTTI* pRtti)
{
  EZ_ASSERT_DEBUG(pRtti != nullptr, "Cannot create object, RTTI type is unknown");
  if (!pRtti->GetAllocator())
    return nullptr;

  void* pObj = pRtti->GetAllocator()->Allocate();
  RegisterObject(guid, pRtti, pObj);
  return pObj;
}

void ezRttiConverterContext::DeleteObject(const ezUuid& guid)
{
  auto object = GetObjectByGUID(guid);
  if (object.m_pObject)
  {
    object.m_pType->GetAllocator()->Deallocate(object.m_pObject);
  }
  UnregisterObject(guid);
}

void ezRttiConverterContext::RegisterObject(const ezUuid& guid, const ezRTTI* pRtti, void* pObject)
{
  EZ_ASSERT_DEV(pObject != nullptr, "cannot register null object!");
  ezRttiConverterObject& co = m_GuidToObject[guid];

  if (pRtti->IsDerivedFrom<ezReflectedClass>())
  {
    pRtti = static_cast<ezReflectedClass*>(pObject)->GetDynamicRTTI();
  }

  //TODO: Actually remove child owner ptr from register when deleting an object
  //EZ_ASSERT_DEV(co.m_pObject == nullptr || (co.m_pObject == pObject && co.m_pType == pRtti), "Registered same guid twice with different values");

  co.m_pObject = pObject;
  co.m_pType = pRtti;

  m_ObjectToGuid[pObject] = guid;
}

void ezRttiConverterContext::UnregisterObject(const ezUuid& guid)
{
  ezRttiConverterObject* pObj;
  if (m_GuidToObject.TryGetValue(guid, pObj))
  {
    m_GuidToObject.Remove(guid);
    m_ObjectToGuid.Remove(pObj->m_pObject);
  }
}

ezRttiConverterObject ezRttiConverterContext::GetObjectByGUID(const ezUuid& guid) const
{
  ezRttiConverterObject object;
  m_GuidToObject.TryGetValue(guid, object);
  return object;
}

ezUuid ezRttiConverterContext::GetObjectGUID(const ezRTTI* pRtti, void* pObject) const
{
  ezUuid guid;

  if (pObject != nullptr)
    m_ObjectToGuid.TryGetValue(pObject, guid);

  return guid;
}

ezUuid ezRttiConverterContext::EnqueObject(const ezUuid& guid, const ezRTTI* pRtti, void* pObject)
{
  EZ_ASSERT_DEBUG(guid.IsValid(), "For stable serialization, guid must be well defined");
  ezUuid res = guid;

  if (pObject != nullptr)
  {
    // In the rare case that this succeeds we already encountered the object with a different guid before.
    // This can happen if two pointer owner point to the same object.
    if (!m_ObjectToGuid.TryGetValue(pObject, res))
    {
      RegisterObject(guid, pRtti, pObject);
    }

    m_QueuedObjects.Insert(res);
  }
  else
  {
    // Replace nullptr with invalid uuid.
    res = ezUuid();
  }
  return res;
}

ezRttiConverterObject ezRttiConverterContext::DequeueObject()
{
  if (!m_QueuedObjects.IsEmpty())
  {
    auto it = m_QueuedObjects.GetIterator();
    auto object = GetObjectByGUID(it.Key());
    EZ_ASSERT_DEV(object.m_pObject != nullptr, "Enqueued object was never registered!");

    m_QueuedObjects.Remove(it);

    return object;
  }

  return ezRttiConverterObject();
}




ezAbstractObjectNode* ezRttiConverterWriter::AddObjectToGraph(const ezRTTI* pRtti, void* pObject, const char* szNodeName)
{
  const ezUuid guid = m_pContext->GetObjectGUID(pRtti, pObject);
  ezAbstractObjectNode* pNode = AddSubObjectToGraph(pRtti, pObject, guid, szNodeName);

  ezRttiConverterObject obj = m_pContext->DequeueObject();
  while (obj.m_pObject != nullptr)
  {
    const ezUuid guid = m_pContext->GetObjectGUID(obj.m_pType, obj.m_pObject);
    AddSubObjectToGraph(obj.m_pType, obj.m_pObject, guid, nullptr);

    obj = m_pContext->DequeueObject();
  }

  return pNode;
}

ezAbstractObjectNode* ezRttiConverterWriter::AddSubObjectToGraph(const ezRTTI* pRtti, void* pObject, const ezUuid& guid, const char* szNodeName)
{
  ezAbstractObjectNode* pNode = m_pGraph->AddNode(guid, pRtti->GetTypeName(), pRtti->GetTypeVersion(), szNodeName);
  AddProperties(pNode, pRtti, pObject);
  return pNode;
}

void ezRttiConverterWriter::AddProperty(ezAbstractObjectNode* pNode, const ezAbstractProperty* pProp, const void* pObject)
{
  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly) && !m_bSerializeReadOnly)
    return;

  if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner) && !m_bSerializeOwnerPtrs)
    return;

  ezVariant vTemp;
  ezStringBuilder sTemp;
  const ezRTTI* pPropType = pProp->GetSpecificType();

  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      const ezAbstractMemberProperty* pSpecific = static_cast<const ezAbstractMemberProperty*>(pProp);

      if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
      {
        vTemp = ezReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
        ezReflectionUtils::EnumerationToString(pPropType, vTemp.Get<ezInt64>(), sTemp);

        pNode->AddProperty(pProp->GetPropertyName(), sTemp.GetData());
      }
      else if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
      {
        pNode->AddProperty(pProp->GetPropertyName(), ezReflectionUtils::GetMemberPropertyValue(pSpecific, pObject));
      }
      else if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        vTemp = ezReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
        void* pRefrencedObject = vTemp.ConvertTo<void*>();

        ezUuid guid = pNode->GetGuid();
        guid.CombineWithSeed(ezUuid::StableUuidForString(pProp->GetPropertyName()));
        if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
        {
          guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          pNode->AddProperty(pProp->GetPropertyName(), guid);
        }
        else
        {
          guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);
          pNode->AddProperty(pProp->GetPropertyName(), guid);
        }
      }
      else if (pPropType->GetProperties().GetCount() > 0)
      {
        void* pSubObject = pSpecific->GetPropertyPointer(pObject);

        ezConversionUtils::ToString(pNode->GetGuid(), sTemp);
        sTemp.Append("/", pProp->GetPropertyName());

        const ezUuid SubObjectGuid = ezUuid::StableUuidForString(sTemp);

        // Do we have direct access to the property?
        if (pSubObject != nullptr)
        {
          pNode->AddProperty(pProp->GetPropertyName(), SubObjectGuid);

          AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);
        }
        // If the property is behind an accessor, we need to retrieve it first.
        else if (pPropType->GetAllocator()->CanAllocate())
        {
          pSubObject = pPropType->GetAllocator()->Allocate();

          pSpecific->GetValuePtr(pObject, pSubObject);

          pNode->AddProperty(pProp->GetPropertyName(), SubObjectGuid);

          AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);

          pPropType->GetAllocator()->Deallocate(pSubObject);
        }
      }
    }
    break;
  case ezPropertyCategory::Array:
    {
      const ezAbstractArrayProperty* pSpecific = static_cast<const ezAbstractArrayProperty*>(pProp);
      ezUInt32 uiCount = pSpecific->GetCount(pObject);
      ezVariantArray values;
      values.SetCount(uiCount);

      if (pSpecific->GetFlags().IsSet(ezPropertyFlags::StandardType))
      {
        for (ezUInt32 i = 0; i < uiCount; ++i)
        {
          values[i] = ezReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
        }
        pNode->AddProperty(pProp->GetPropertyName(), values);
      }
      else if (pSpecific->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        ezUuid propertyGuid = pNode->GetGuid();
        propertyGuid.CombineWithSeed(ezUuid::StableUuidForString(pProp->GetPropertyName()));
        for (ezUInt32 i = 0; i < uiCount; ++i)
        {
          vTemp = ezReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
          void* pRefrencedObject = vTemp.ConvertTo<void*>();

          ezUuid guid;
          if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
          {
            guid = propertyGuid;
            guid.CombineWithSeed(ezUuid::StableUuidForInt(i));
            guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          }
          else
            guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);

          values[i] = guid;
        }

        pNode->AddProperty(pProp->GetPropertyName(), values);
      }
      else if (pPropType->GetAllocator()->CanAllocate())
      {
        void* pSubObject = pPropType->GetAllocator()->Allocate();

        for (ezUInt32 i = 0; i < uiCount; ++i)
        {
          pSpecific->GetValue(pObject, i, pSubObject);

          ezConversionUtils::ToString(pNode->GetGuid(), sTemp);
          sTemp.AppendFormat("/{0}/{1}", pProp->GetPropertyName(), i);
          const ezUuid SubObjectGuid = ezUuid::StableUuidForString(sTemp);
          AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);

          values[i] = SubObjectGuid;
        }
        pNode->AddProperty(pProp->GetPropertyName(), values);
        pPropType->GetAllocator()->Deallocate(pSubObject);
      }
    }
    break;
  case ezPropertyCategory::Set:
    {
      const ezAbstractSetProperty* pSpecific = static_cast<const ezAbstractSetProperty*>(pProp);

      ezHybridArray<ezVariant, 16> values;
      pSpecific->GetValues(pObject, values);

      ezVariantArray ValuesCopied(values);

      if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
      {
        pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
      }
      else if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        ezUuid propertyGuid = pNode->GetGuid();
        propertyGuid.CombineWithSeed(ezUuid::StableUuidForString(pProp->GetPropertyName()));
        for (ezUInt32 i = 0; i < values.GetCount(); ++i)
        {
          void* pRefrencedObject = values[i].ConvertTo<void*>();

          ezUuid guid;
          if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
          {
            // TODO: pointer sets are never stable unless they use an array based pseudo set as storage.
            guid = propertyGuid;
            guid.CombineWithSeed(ezUuid::StableUuidForInt(i));
            guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          }
          else
            guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);

          ValuesCopied[i] = guid;
        }

        pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
      }
    }
    break;

  default:
    break;
  }
}

void ezRttiConverterWriter::AddProperties(ezAbstractObjectNode* pNode, const ezRTTI* pRtti, const void* pObject)
{
  if (pRtti->GetParentType())
    AddProperties(pNode, pRtti->GetParentType(), pObject);

  for (const auto* pProp : pRtti->GetProperties())
  {
    AddProperty(pNode, pProp, pObject);
  }
}



EZ_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_RttiConverterWriter);

