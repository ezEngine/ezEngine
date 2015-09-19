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
          if (guid.IsValid())
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
        pNode->AddProperty(pProp->GetPropertyName(), SubObjectGuid);
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
        if (!pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
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



ezDocumentObjectConverterReader::ezDocumentObjectConverterReader(const ezAbstractObjectGraph* pGraph, ezDocumentObjectManager* pManager, Mode mode)
{
  m_pManager = pManager;
  m_pGraph = pGraph;
  m_Mode = mode;
}

ezDocumentObjectBase* ezDocumentObjectConverterReader::CreateObjectFromNode(const ezAbstractObjectNode* pNode, ezDocumentObjectBase* pParent, const char* szParentProperty, ezVariant index)
{
  ezDocumentObjectBase* pObject = m_pManager->CreateObject(ezRTTI::FindTypeByName(pNode->GetType()), pNode->GetGuid());

  switch (m_Mode)
  {
  case ezDocumentObjectConverterReader::Mode::CreateOnly:
    if (pParent != nullptr)
      pParent->InsertSubObject(pObject, szParentProperty, index);
    break;
  case ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument:
    m_pManager->AddObject(pObject, pParent, szParentProperty, index);
    break;
  case ezDocumentObjectConverterReader::Mode::CreateAndAddToDocumentUndoable:
    // TODO:
    m_pManager->AddObject(pObject, pParent, szParentProperty, index); 
    break;
  }
  
  return pObject;
}

void ezDocumentObjectConverterReader::ApplyPropertiesToObject(const ezAbstractObjectNode* pNode, ezDocumentObjectBase* pObject)
{
  ezHybridArray<ezAbstractProperty*, 32> Properties;
  pObject->GetTypeAccessor().GetType()->GetAllProperties(Properties);

  for (auto* pProp : Properties)
  {
    auto* pOtherProp = pNode->FindProperty(pProp->GetPropertyName());
    if (pOtherProp == nullptr)
      continue;

    ApplyProperty(pObject, pProp, pOtherProp);
  }
}

void ezDocumentObjectConverterReader::ApplyProperty(ezDocumentObjectBase* pObject, ezAbstractProperty* pProp, const ezAbstractObjectNode::Property* pSource)
{
  const ezRTTI* pPropType = pProp->GetSpecificType();
  const ezPropertyPath path(pProp->GetPropertyName());
  ezStringBuilder sTemp;

  if (pProp->GetCategory() == ezPropertyCategory::Member)
  {
    if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
    {
      const ezUuid guid = pSource->m_Value.Get<ezUuid>();
      if (guid.IsValid())
      {
        auto* pSubNode = m_pGraph->GetNode(guid);
        EZ_ASSERT_DEV(pSubNode != nullptr, "invalid document");

        auto* pSubObject = CreateObjectFromNode(pSubNode, pObject, pProp->GetPropertyName(), ezVariant());
        ApplyPropertiesToObject(pSubNode, pSubObject);
      }
    }
    else if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags | ezPropertyFlags::StandardType | ezPropertyFlags::Pointer))
    {
      pObject->GetTypeAccessor().SetValue(path, pSource->m_Value);
    }
    else
    {
      const ezUuid guid = pSource->m_Value.Get<ezUuid>();

      ezDocumentSubObject SubObj(pPropType);
      SubObj.SetObject(pObject, path, guid);

      auto* pSubNode = m_pGraph->GetNode(guid);
      EZ_ASSERT_DEV(pSubNode != nullptr, "invalid document");

      ApplyPropertiesToObject(pSubNode, &SubObj);
    }
  }
  else if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
  {
    const ezVariantArray& array = pSource->m_Value.Get<ezVariantArray>();

    if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType | ezPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
    {
      for (ezUInt32 i = 0; i < array.GetCount(); ++i)
      {
        pObject->GetTypeAccessor().InsertValue(path, i, array[i]);
      }
    }
    else
    {
      ezInt32 iCurrentCount = pObject->GetTypeAccessor().GetCount(path);
      for (ezUInt32 i = 0; i < array.GetCount(); ++i)
      {
        const ezUuid guid = array[i].Get<ezUuid>();

        if (guid.IsValid())
        {
          auto* pSubNode = m_pGraph->GetNode(guid);
          EZ_ASSERT_DEV(pSubNode != nullptr, "invalid document");

          ezDocumentObjectBase* pSubObject = nullptr;
          if (i < (ezUInt32)iCurrentCount)
          {
            // Overwrite existing object
            ezUuid childGuid = pObject->GetTypeAccessor().GetValue(path, i).ConvertTo<ezUuid>();
            pSubObject = m_pManager->GetObject(childGuid);
          }
          else
          {
            pSubObject = CreateObjectFromNode(pSubNode, pObject, pProp->GetPropertyName(), -1);
          }

          ApplyPropertiesToObject(pSubNode, pSubObject);
        }
      }
    }
  }
}

