#include <PCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/RttiConverter.h>

ezRttiConverterReader::ezRttiConverterReader(const ezAbstractObjectGraph* pGraph, ezRttiConverterContext* pContext)
{
  m_pGraph = pGraph;
  m_pContext = pContext;
}

void* ezRttiConverterReader::CreateObjectFromNode(const ezAbstractObjectNode* pNode)
{
  const ezRTTI* pRtti = ezRTTI::FindTypeByName(pNode->GetType());
  if (pRtti == nullptr)
  {
    ezLog::Error("RTTI type '{0}' is unknown, CreateObjectFromNode failed.", pNode->GetType());
    return nullptr;
  }

  void* pObject = m_pContext->CreateObject(pNode->GetGuid(), pRtti);
  if (pObject)
  {
    ApplyPropertiesToObject(pNode, pRtti, pObject);
  }

  CallOnObjectCreated(pNode, pRtti, pObject);
  return pObject;
}

void ezRttiConverterReader::ApplyPropertiesToObject(const ezAbstractObjectNode* pNode, const ezRTTI* pRtti, void* pObject)
{
  EZ_ASSERT_DEBUG(pNode != nullptr, "Invalid node");

  if (pRtti->GetParentType() != nullptr)
    ApplyPropertiesToObject(pNode, pRtti->GetParentType(), pObject);

  for (auto* prop : pRtti->GetProperties())
  {
    auto* pOtherProp = pNode->FindProperty(prop->GetPropertyName());
    if (pOtherProp == nullptr)
      continue;

    ApplyProperty(pObject, prop, pOtherProp);
  }
}

void ezRttiConverterReader::ApplyProperty(void* pObject, ezAbstractProperty* pProp, const ezAbstractObjectNode::Property* pSource)
{
  const ezRTTI* pPropType = pProp->GetSpecificType();

  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
    return;

  switch (pProp->GetCategory())
  {
    case ezPropertyCategory::Member:
    {
      ezAbstractMemberProperty* pSpecific = static_cast<ezAbstractMemberProperty*>(pProp);

      if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        if (!pSource->m_Value.IsA<ezUuid>())
          return;

        ezUuid guid = pSource->m_Value.Get<ezUuid>();
        void* pRefrencedObject = nullptr;

        if (guid.IsValid())
        {
          if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
          {
            auto* pNode = m_pGraph->GetNode(guid);
            EZ_ASSERT_DEV(pNode != nullptr, "node must exist");
            pRefrencedObject = CreateObjectFromNode(pNode);
            if (pRefrencedObject == nullptr)
            {
              // ezLog::Error("Failed to set property '{0}', type could not be created!", pProp->GetPropertyName());
              return;
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
        }

        void* pOldObject = nullptr;
        pSpecific->GetValuePtr(pObject, &pOldObject);
        pSpecific->SetValuePtr(pObject, &pRefrencedObject);
        if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
          ezReflectionUtils::DeleteObject(pOldObject, pProp);
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType | ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
        {
          ezReflectionUtils::SetMemberPropertyValue(pSpecific, pObject, pSource->m_Value);
        }
        else if (pProp->GetFlags().IsSet(ezPropertyFlags::Class))
        {
          if (!pSource->m_Value.IsA<ezUuid>())
            return;

          void* pDirectPtr = pSpecific->GetPropertyPointer(pObject);
          bool bDelete = false;
          const ezUuid sourceGuid = pSource->m_Value.Get<ezUuid>();

          if (pDirectPtr == nullptr)
          {
            bDelete = true;
            pDirectPtr = m_pContext->CreateObject(sourceGuid, pPropType);
          }

          auto* pNode = m_pGraph->GetNode(sourceGuid);
          EZ_ASSERT_DEV(pNode != nullptr, "node must exist");

          ApplyPropertiesToObject(pNode, pPropType, pDirectPtr);

          if (bDelete)
          {
            pSpecific->SetValuePtr(pObject, pDirectPtr);
            m_pContext->DeleteObject(sourceGuid);
          }
        }
      }
    }
    break;
    case ezPropertyCategory::Array:
    {
      ezAbstractArrayProperty* pSpecific = static_cast<ezAbstractArrayProperty*>(pProp);
      if (!pSource->m_Value.IsA<ezVariantArray>())
        return;
      const ezVariantArray& array = pSource->m_Value.Get<ezVariantArray>();
      // Delete old values
      if (pProp->GetFlags().AreAllSet(ezPropertyFlags::Pointer | ezPropertyFlags::PointerOwner))
      {
        const ezInt32 uiOldCount = (ezInt32)pSpecific->GetCount(pObject);
        for (ezInt32 i = uiOldCount - 1; i >= 0; --i)
        {
          void* pOldObject = nullptr;
          pSpecific->GetValue(pObject, i, &pOldObject);
          pSpecific->Remove(pObject, i);
          if (pOldObject)
            ezReflectionUtils::DeleteObject(pOldObject, pProp);
        }
      }

      pSpecific->SetCount(pObject, array.GetCount());
      if (pProp->GetFlags().IsAnySet(ezPropertyFlags::Pointer))
      {
        for (ezUInt32 i = 0; i < array.GetCount(); ++i)
        {
          if (!array[i].IsA<ezUuid>())
            continue;
          ezUuid guid = array[i].Get<ezUuid>();
          void* pRefrencedObject = nullptr;
          if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
          {
            if (guid.IsValid())
            {
              auto* pNode = m_pGraph->GetNode(guid);
              EZ_ASSERT_DEV(pNode != nullptr, "node must exist");
              pRefrencedObject = CreateObjectFromNode(pNode);
              if (pRefrencedObject == nullptr)
              {
                ezLog::Error("Failed to set array property '{0}' element, type could not be created!", pProp->GetPropertyName());
                continue;
              }
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
          pSpecific->SetValue(pObject, i, &pRefrencedObject);
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType))
        {
          for (ezUInt32 i = 0; i < array.GetCount(); ++i)
          {
            ezReflectionUtils::SetArrayPropertyValue(pSpecific, pObject, i, array[i]);
          }
        }
        else if (pProp->GetFlags().IsAnySet(ezPropertyFlags::Class))
        {
          ezUuid temp;
          temp.CreateNewUuid();
          void* pValuePtr = m_pContext->CreateObject(temp, pPropType);

          for (ezUInt32 i = 0; i < array.GetCount(); ++i)
          {
            if (!array[i].IsA<ezUuid>())
              continue;

            const ezUuid sourceGuid = array[i].Get<ezUuid>();
            auto* pNode = m_pGraph->GetNode(sourceGuid);
            EZ_ASSERT_DEV(pNode != nullptr, "node must exist");

            ApplyPropertiesToObject(pNode, pPropType, pValuePtr);
            pSpecific->SetValue(pObject, i, pValuePtr);
          }

          m_pContext->DeleteObject(temp);
        }
      }
    }
    break;
    case ezPropertyCategory::Set:
    {
      ezAbstractSetProperty* pSpecific = static_cast<ezAbstractSetProperty*>(pProp);
      if (!pSource->m_Value.IsA<ezVariantArray>())
        return;

      const ezVariantArray& array = pSource->m_Value.Get<ezVariantArray>();

      // Delete old values
      if (pProp->GetFlags().AreAllSet(ezPropertyFlags::Pointer | ezPropertyFlags::PointerOwner))
      {
        ezHybridArray<ezVariant, 16> keys;
        pSpecific->GetValues(pObject, keys);
        pSpecific->Clear(pObject);
        for (ezVariant& value : keys)
        {
          void* pOldObject = value.ConvertTo<void*>();
          if (pOldObject)
            ezReflectionUtils::DeleteObject(pOldObject, pProp);
        }
      }

      pSpecific->Clear(pObject);

      if (pProp->GetFlags().IsAnySet(ezPropertyFlags::Pointer))
      {
        for (ezUInt32 i = 0; i < array.GetCount(); ++i)
        {
          if (!array[i].IsA<ezUuid>())
            continue;

          ezUuid guid = array[i].Get<ezUuid>();
          void* pRefrencedObject = nullptr;
          if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
          {
            auto* pNode = m_pGraph->GetNode(guid);
            EZ_ASSERT_DEV(pNode != nullptr, "node must exist");
            pRefrencedObject = CreateObjectFromNode(pNode);
            if (pRefrencedObject == nullptr)
            {
              ezLog::Error("Failed to insert set element in to property '{0}', type could not be created!", pProp->GetPropertyName());
              continue;
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
          pSpecific->Insert(pObject, &pRefrencedObject);
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType))
        {
          for (ezUInt32 i = 0; i < array.GetCount(); ++i)
          {
            ezReflectionUtils::InsertSetPropertyValue(pSpecific, pObject, array[i]);
          }
        }
        else if (pProp->GetFlags().IsAnySet(ezPropertyFlags::Class))
        {
          ezUuid temp;
          temp.CreateNewUuid();
          void* pValuePtr = m_pContext->CreateObject(temp, pPropType);

          for (ezUInt32 i = 0; i < array.GetCount(); ++i)
          {
            if (!array[i].IsA<ezUuid>())
              continue;

            const ezUuid sourceGuid = array[i].Get<ezUuid>();
            auto* pNode = m_pGraph->GetNode(sourceGuid);
            EZ_ASSERT_DEV(pNode != nullptr, "node must exist");

            ApplyPropertiesToObject(pNode, pPropType, pValuePtr);
            pSpecific->Insert(pObject, pValuePtr);
          }

          m_pContext->DeleteObject(temp);
        }
      }
    }
    break;
    case ezPropertyCategory::Map:
    {
      ezAbstractMapProperty* pSpecific = static_cast<ezAbstractMapProperty*>(pProp);
      if (!pSource->m_Value.IsA<ezVariantDictionary>())
        return;

      const ezVariantDictionary& dict = pSource->m_Value.Get<ezVariantDictionary>();

      // Delete old values
      if (pProp->GetFlags().AreAllSet(ezPropertyFlags::Pointer | ezPropertyFlags::PointerOwner))
      {
        ezHybridArray<ezString, 16> keys;
        pSpecific->GetKeys(pObject, keys);
        for (const ezString& sKey : keys)
        {
          ezVariant value = ezReflectionUtils::GetMapPropertyValue(pSpecific, pObject, sKey);
          void* pOldClone = value.ConvertTo<void*>();
          pSpecific->Remove(pObject, sKey);
          if (pOldClone)
            ezReflectionUtils::DeleteObject(pOldClone, pProp);
        }
      }

      pSpecific->Clear(pObject);

      if (pProp->GetFlags().IsAnySet(ezPropertyFlags::Pointer))
      {
        for (auto it = dict.GetIterator(); it.IsValid(); ++it)
        {
          if (!it.Value().IsA<ezUuid>())
            continue;

          ezUuid guid = it.Value().Get<ezUuid>();
          void* pRefrencedObject = nullptr;
          if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
          {
            if (guid.IsValid())
            {
              auto* pNode = m_pGraph->GetNode(guid);
              EZ_ASSERT_DEV(pNode != nullptr, "node must exist");
              pRefrencedObject = CreateObjectFromNode(pNode);
              if (pRefrencedObject == nullptr)
              {
                ezLog::Error("Failed to insert set element in to property '{0}', type could not be created!", pProp->GetPropertyName());
                continue;
              }
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
          pSpecific->Insert(pObject, it.Key(), &pRefrencedObject);
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType))
        {
          for (auto it = dict.GetIterator(); it.IsValid(); ++it)
          {
            ezReflectionUtils::SetMapPropertyValue(pSpecific, pObject, it.Key(), it.Value());
          }
        }
        else if (pProp->GetFlags().IsAnySet(ezPropertyFlags::Class))
        {
          ezUuid temp;
          temp.CreateNewUuid();
          void* pValuePtr = m_pContext->CreateObject(temp, pPropType);

          for (auto it = dict.GetIterator(); it.IsValid(); ++it)
          {
            if (!it.Value().IsA<ezUuid>())
              continue;

            const ezUuid sourceGuid = it.Value().Get<ezUuid>();
            auto* pNode = m_pGraph->GetNode(sourceGuid);
            EZ_ASSERT_DEV(pNode != nullptr, "node must exist");

            ApplyPropertiesToObject(pNode, pPropType, pValuePtr);
            pSpecific->Insert(pObject, it.Key(), pValuePtr);
          }

          m_pContext->DeleteObject(temp);
        }
      }
    }
    break;
  }
}

void ezRttiConverterReader::CallOnObjectCreated(const ezAbstractObjectNode* pNode, const ezRTTI* pRtti, void* pObject)
{
  ezArrayPtr<ezAbstractFunctionProperty*> functions = pRtti->GetFunctions();
  for (ezAbstractFunctionProperty* pFunc : functions)
  {
    // TODO: Make this compare faster
    if (ezStringUtils::IsEqual(pFunc->GetPropertyName(), "OnObjectCreated"))
    {
      ezHybridArray<ezVariant, 1> params;
      params.PushBack(pNode);
      ezVariant ret;
      pFunc->Execute(pObject, params, ret);
    }
  }
}

EZ_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_RttiConverterReader);
