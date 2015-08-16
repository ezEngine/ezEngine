#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

ezAbstractObjectNode* ezDocumentObjectConverterWriter::AddObjectToGraph(const ezDocumentObjectBase* pObject, const char* szNodeName)
{
  ezAbstractObjectNode* pNode = AddSubObjectToGraph(pObject, szNodeName);

  while (!m_QueuedObjects.IsEmpty())
  {
    auto itCur = m_QueuedObjects.GetIterator();

    AddSubObjectToGraph(itCur.Key(), nullptr);

    m_QueuedObjects.Remove(itCur);
  }

  return pNode;
}

void ezDocumentObjectConverterWriter::AddProperty(ezAbstractObjectNode* pNode, const ezAbstractProperty* pProp, const ezDocumentObjectBase* pObject)
{
  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly) && !m_bSerializeReadOnly)
    return;

  if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner) && !m_bSerializeOwnerPtrs)
    return;

  const ezRTTI* pPropType = pProp->GetSpecificType();

  const ezPropertyPath path(pProp->GetPropertyName());
  ezStringBuilder sTemp;

  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
      {
        ezReflectionUtils::EnumerationToString(pPropType, pObject->GetTypeAccessor().GetValue(path).ConvertTo<ezInt64>(), sTemp);

        pNode->AddProperty(pProp->GetPropertyName(), sTemp.GetData());
      }
      else if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
      {
        pNode->AddProperty(pProp->GetPropertyName(), pObject->GetTypeAccessor().GetValue(path));
      }
      else if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
        {
          const ezUuid guid = pObject->GetTypeAccessor().GetValue(path).Get<ezUuid>();

          pNode->AddProperty(pProp->GetPropertyName(), guid);
          m_QueuedObjects.Insert(m_pManager->GetObject(guid));
        }
        else
        {
          pNode->AddProperty(pProp->GetPropertyName(), pObject->GetTypeAccessor().GetValue(path));
        }
      }
      else
      {
        sTemp = ezConversionUtils::ToString(pNode->GetGuid());
        sTemp.Append("/", pProp->GetPropertyName());

        const ezUuid SubObjectGuid = ezUuid::StableUuidForString(sTemp);

        ezDocumentSubObject SubObj(pPropType);
        SubObj.SetObject(const_cast<ezDocumentObjectBase*>(pObject), path, SubObjectGuid);
       
        AddSubObjectToGraph(&SubObj, nullptr);
      }
    }

    break;

  case ezPropertyCategory::Array:
  case ezPropertyCategory::Set:
    {
      const ezInt32 iCount = pObject->GetTypeAccessor().GetCount(path);
      EZ_ASSERT_DEV(iCount >= 0, "Invalid array property size %i", iCount);

      ezVariantArray values;
      values.SetCount(iCount);

      for (ezInt32 i = 0; i < iCount; ++i)
      {
        values[i] = pObject->GetTypeAccessor().GetValue(path, i);
        if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
        {
          m_QueuedObjects.Insert(m_pManager->GetObject(values[i].Get<ezUuid>()));
        }
      }
      pNode->AddProperty(pProp->GetPropertyName(), values);
    }
    break;
  }
}

void ezDocumentObjectConverterWriter::AddProperties(ezAbstractObjectNode* pNode, const ezDocumentObjectBase* pObject)
{
  ezHybridArray<ezAbstractProperty*, 32> Properties;
  pObject->GetTypeAccessor().GetType()->GetAllProperties(Properties);

  for (const auto* pProp : Properties)
  {
    AddProperty(pNode, pProp, pObject);
  }
}

ezAbstractObjectNode* ezDocumentObjectConverterWriter::AddSubObjectToGraph(const ezDocumentObjectBase* pObject, const char* szNodeName)
{
  ezAbstractObjectNode* pNode = m_pGraph->AddNode(pObject->GetGuid(), pObject->GetTypeAccessor().GetType()->GetTypeName(), szNodeName);
  AddProperties(pNode, pObject);
  return pNode;
}



