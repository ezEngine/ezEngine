#include <Foundation/PCH.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Reflection/ReflectionUtils.h>

ezRttiConverterReader::ezRttiConverterReader(const ezAbstractObjectGraph* pGraph, ezRttiConverterContext* pContext)
{
  m_pGraph = pGraph;
  m_pContext = pContext;
}

void* ezRttiConverterReader::CreateObjectFromNode(const ezAbstractObjectNode* pNode)
{
  const ezRTTI* pRtti = ezRTTI::FindTypeByName(pNode->GetType());
  void* pObject = m_pContext->CreateObject(pNode->GetGuid(), pRtti);

  ApplyPropertiesToObject(pNode, pRtti, pObject);
  return pObject;
}

void ezRttiConverterReader::ApplyPropertiesToObject(const ezAbstractObjectNode* pNode, const ezRTTI* pRtti, void* pObject)
{
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

  if (pProp->GetCategory() == ezPropertyCategory::Member)
  {
    ezAbstractMemberProperty* pSpecific = static_cast<ezAbstractMemberProperty*>(pProp);

    if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType | ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
    {
      ezReflectionUtils::SetMemberPropertyValue(pSpecific, pObject, pSource->m_Value);
    }
    else if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
    {
      ezUuid guid = pSource->m_Value.Get<ezUuid>();
      void* pRefrencedObject = nullptr;

      if (guid.IsValid())
      {
        if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
        {
          auto* pNode = m_pGraph->GetNode(guid);
          EZ_ASSERT_DEV(pNode != nullptr, "node must exist");
          pRefrencedObject = CreateObjectFromNode(pNode);
        }
        else
        {
          pRefrencedObject = m_pContext->GetObjectByGUID(guid)->m_pObject;
        }
      }

      pSpecific->SetValuePtr(pObject, &pRefrencedObject);
    }
    else
    {
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
  else if (pProp->GetCategory() == ezPropertyCategory::Array)
  {
    ezAbstractArrayProperty* pSpecific = static_cast<ezAbstractArrayProperty*>(pProp);

    const ezVariantArray& array = pSource->m_Value.Get<ezVariantArray>();

    pSpecific->SetCount(pObject, array.GetCount());
    if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType))
    {
      for (ezUInt32 i = 0; i < array.GetCount(); ++i)
      {
        ezReflectionUtils::SetArrayPropertyValue(pSpecific, pObject, i, array[i]);
      }
    }
    else if (pProp->GetFlags().IsAnySet(ezPropertyFlags::Pointer))
    {
      for (ezUInt32 i = 0; i < array.GetCount(); ++i)
      {
        ezUuid guid = array[i].Get<ezUuid>();
        void* pRefrencedObject = nullptr;
        if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
        {
          auto* pNode = m_pGraph->GetNode(guid);
          EZ_ASSERT_DEV(pNode != nullptr, "node must exist");
          pRefrencedObject = CreateObjectFromNode(pNode);
        }
        else
        {
          pRefrencedObject = m_pContext->GetObjectByGUID(guid)->m_pObject;
        }
        pSpecific->SetValue(pObject, i, &pRefrencedObject);
      }
    }
    else
    {
      ezUuid temp;
      temp.CreateNewUuid();
      void* pValuePtr = m_pContext->CreateObject(temp, pPropType);
      
      for (ezUInt32 i = 0; i < array.GetCount(); ++i)
      {
        const ezUuid sourceGuid = array[i].Get<ezUuid>();
        auto* pNode = m_pGraph->GetNode(sourceGuid);
        EZ_ASSERT_DEV(pNode != nullptr, "node must exist");

        ApplyPropertiesToObject(pNode, pPropType, pValuePtr);
        pSpecific->SetValue(pObject, i, pValuePtr);
      }

      m_pContext->DeleteObject(temp);
    }
  }
  else if (pProp->GetCategory() == ezPropertyCategory::Set)
  {
    ezAbstractSetProperty* pSpecific = static_cast<ezAbstractSetProperty*>(pProp);

    const ezVariantArray& array = pSource->m_Value.Get<ezVariantArray>();

    pSpecific->Clear(pObject);
    if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType))
    {
      for (ezUInt32 i = 0; i < array.GetCount(); ++i)
      {
        ezReflectionUtils::InsertSetPropertyValue(pSpecific, pObject, array[i]);
      }
    }
    else if (pProp->GetFlags().IsAnySet(ezPropertyFlags::Pointer))
    {
      for (ezUInt32 i = 0; i < array.GetCount(); ++i)
      {
        ezUuid guid = array[i].Get<ezUuid>();
        void* pRefrencedObject = nullptr;
        if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
        {
          auto* pNode = m_pGraph->GetNode(guid);
          EZ_ASSERT_DEV(pNode != nullptr, "node must exist");
          pRefrencedObject = CreateObjectFromNode(pNode);
        }
        else
        {
          pRefrencedObject = m_pContext->GetObjectByGUID(guid)->m_pObject;
        }
        pSpecific->Insert(pObject, &pRefrencedObject);
      }
    }
    else
    {
      ezUuid temp;
      temp.CreateNewUuid();
      void* pValuePtr = m_pContext->CreateObject(temp, pPropType);

      for (ezUInt32 i = 0; i < array.GetCount(); ++i)
      {
        const ezUuid sourceGuid = pSource->m_Value.Get<ezUuid>();
        auto* pNode = m_pGraph->GetNode(sourceGuid);
        EZ_ASSERT_DEV(pNode != nullptr, "node must exist");

        ApplyPropertiesToObject(pNode, pPropType, pValuePtr);
        pSpecific->Insert(pObject, pValuePtr);
      }

      m_pContext->DeleteObject(temp);
    }
  }
}


