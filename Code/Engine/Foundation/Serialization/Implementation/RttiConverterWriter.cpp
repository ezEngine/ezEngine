#include <FoundationPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>

void ezRttiConverterContext::Clear()
{
  m_GuidToObject.Clear();
  m_ObjectToGuid.Clear();
  m_QueuedObjects.Clear();
}

ezUuid ezRttiConverterContext::GenerateObjectGuid(const ezUuid& parentGuid, const ezAbstractProperty* pProp, ezVariant index, void* pObject) const
{
  ezUuid guid = parentGuid;
  guid.HashCombine(ezUuid::StableUuidForString(pProp->GetPropertyName()));
  if (index.IsA<ezString>())
  {
    guid.HashCombine(ezUuid::StableUuidForString(index.Get<ezString>()));
  }
  else if (index.CanConvertTo<ezUInt32>())
  {
    guid.HashCombine(ezUuid::StableUuidForInt(index.ConvertTo<ezUInt32>()));
  }
  else if (index.IsValid())
  {
    EZ_REPORT_FAILURE("Index type must be ezUInt32 or ezString.");
  }
  // ezLog::Warning("{0},{1},{2} -> {3}", parentGuid, pProp->GetPropertyName(), index, guid);
  return guid;
}

void* ezRttiConverterContext::CreateObject(const ezUuid& guid, const ezRTTI* pRtti)
{
  EZ_ASSERT_DEBUG(pRtti != nullptr, "Cannot create object, RTTI type is unknown");
  if (!pRtti->GetAllocator() || !pRtti->GetAllocator()->CanAllocate())
    return nullptr;

  void* pObj = pRtti->GetAllocator()->Allocate<void>();
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

  // TODO: Actually remove child owner ptr from register when deleting an object
  // EZ_ASSERT_DEV(co.m_pObject == nullptr || (co.m_pObject == pObject && co.m_pType == pRtti), "Registered same guid twice with different
  // values");

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

ezUuid ezRttiConverterContext::GetObjectGUID(const ezRTTI* pRtti, const void* pObject) const
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



ezAbstractObjectNode* ezRttiConverterWriter::AddObjectToGraph(const ezRTTI* pRtti, const void* pObject, const char* szNodeName)
{
  const ezUuid guid = m_pContext->GetObjectGUID(pRtti, pObject);
  EZ_ASSERT_DEV(guid.IsValid(), "The object was not registered. Call ezRttiConverterContext::RegisterObject before adding.");
  ezAbstractObjectNode* pNode = AddSubObjectToGraph(pRtti, pObject, guid, szNodeName);

  ezRttiConverterObject obj = m_pContext->DequeueObject();
  while (obj.m_pObject != nullptr)
  {
    const ezUuid objectGuid = m_pContext->GetObjectGUID(obj.m_pType, obj.m_pObject);
    AddSubObjectToGraph(obj.m_pType, obj.m_pObject, objectGuid, nullptr);

    obj = m_pContext->DequeueObject();
  }

  return pNode;
}

ezAbstractObjectNode* ezRttiConverterWriter::AddSubObjectToGraph(const ezRTTI* pRtti, const void* pObject, const ezUuid& guid, const char* szNodeName)
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

      if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        vTemp = ezReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
        void* pRefrencedObject = vTemp.ConvertTo<void*>();

        ezUuid guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, ezVariant(), pRefrencedObject);
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
      else
      {
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
        else if (pProp->GetFlags().IsSet(ezPropertyFlags::Class) && pPropType->GetProperties().GetCount() > 0)
        {
          void* pSubObject = pSpecific->GetPropertyPointer(pObject);


          // Do we have direct access to the property?
          if (pSubObject != nullptr)
          {
            const ezUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, ezVariant(), pSubObject);
            pNode->AddProperty(pProp->GetPropertyName(), SubObjectGuid);

            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);
          }
          // If the property is behind an accessor, we need to retrieve it first.
          else if (pPropType->GetAllocator()->CanAllocate())
          {
            pSubObject = pPropType->GetAllocator()->Allocate<void>();

            pSpecific->GetValuePtr(pObject, pSubObject);
            const ezUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, ezVariant(), pSubObject);
            pNode->AddProperty(pProp->GetPropertyName(), SubObjectGuid);

            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);

            pPropType->GetAllocator()->Deallocate(pSubObject);
          }
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

      if (pSpecific->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        for (ezUInt32 i = 0; i < uiCount; ++i)
        {
          vTemp = ezReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
          void* pRefrencedObject = vTemp.ConvertTo<void*>();

          ezUuid guid;
          if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
          {
            guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, i, pRefrencedObject);
            guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          }
          else
            guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);

          values[i] = guid;
        }

        pNode->AddProperty(pProp->GetPropertyName(), values);
      }
      else
      {
        if (pSpecific->GetFlags().IsSet(ezPropertyFlags::StandardType))
        {
          for (ezUInt32 i = 0; i < uiCount; ++i)
          {
            values[i] = ezReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
          }
          pNode->AddProperty(pProp->GetPropertyName(), values);
        }
        else if (pSpecific->GetFlags().IsSet(ezPropertyFlags::Class) && pPropType->GetAllocator()->CanAllocate())
        {
          void* pSubObject = pPropType->GetAllocator()->Allocate<void>();

          for (ezUInt32 i = 0; i < uiCount; ++i)
          {
            pSpecific->GetValue(pObject, i, pSubObject);
            const ezUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, i, pSubObject);
            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);

            values[i] = SubObjectGuid;
          }
          pNode->AddProperty(pProp->GetPropertyName(), values);
          pPropType->GetAllocator()->Deallocate(pSubObject);
        }
      }
    }
    break;
    case ezPropertyCategory::Set:
    {
      const ezAbstractSetProperty* pSpecific = static_cast<const ezAbstractSetProperty*>(pProp);

      ezHybridArray<ezVariant, 16> values;
      pSpecific->GetValues(pObject, values);

      ezVariantArray ValuesCopied(values);

      if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        for (ezUInt32 i = 0; i < values.GetCount(); ++i)
        {
          void* pRefrencedObject = values[i].ConvertTo<void*>();

          ezUuid guid;
          if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
          {
            // TODO: pointer sets are never stable unless they use an array based pseudo set as storage.
            guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, i, pRefrencedObject);
            guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          }
          else
            guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);

          ValuesCopied[i] = guid;
        }

        pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
      }
      else
      {
        if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
        {
          pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
        }
      }
    }
    break;
    case ezPropertyCategory::Map:
    {
      const ezAbstractMapProperty* pSpecific = static_cast<const ezAbstractMapProperty*>(pProp);

      ezHybridArray<ezString, 16> keys;
      pSpecific->GetKeys(pObject, keys);

      ezVariantDictionary ValuesCopied;
      ValuesCopied.Reserve(keys.GetCount());

      if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        for (ezUInt32 i = 0; i < keys.GetCount(); ++i)
        {
          ezVariant value = ezReflectionUtils::GetMapPropertyValue(pSpecific, pObject, keys[i]);
          void* pRefrencedObject = value.ConvertTo<void*>();

          ezUuid guid;
          if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
          {
            guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, ezVariant(keys[i]), pRefrencedObject);
            guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          }
          else
            guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);

          ValuesCopied.Insert(keys[i], guid);
        }

        pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
      }
      else
      {
        if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
        {
          for (ezUInt32 i = 0; i < keys.GetCount(); ++i)
          {
            ezVariant value = ezReflectionUtils::GetMapPropertyValue(pSpecific, pObject, keys[i]);
            ValuesCopied.Insert(keys[i], value);
          }
          pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
        }
        else if (pProp->GetFlags().IsSet(ezPropertyFlags::Class))
        {
          for (ezUInt32 i = 0; i < keys.GetCount(); ++i)
          {
            void* pSubObject = pPropType->GetAllocator()->Allocate<void>();
            EZ_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(pSubObject););
            EZ_VERIFY(pSpecific->GetValue(pObject, keys[i], pSubObject), "Key should be valid.");

            const ezUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, ezVariant(keys[i]), pSubObject);
            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);
            ValuesCopied.Insert(keys[i], SubObjectGuid);
          }
          pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
        }
      }
    }
    break;
    case ezPropertyCategory::Constant:
      // Nothing to do here.
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
