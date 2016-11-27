#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <Foundation/Logging/Log.h>

ezAbstractObjectNode* ezDocumentObjectConverterWriter::AddObjectToGraph(const ezDocumentObject* pObject, const char* szNodeName)
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

void ezDocumentObjectConverterWriter::AddProperty(ezAbstractObjectNode* pNode, const ezAbstractProperty* pProp, const ezDocumentObject* pObject)
{
  if (pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly) && !m_bSerializeReadOnly)
    return;

  if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner) && !m_bSerializeOwnerPtrs)
    return;

  const ezRTTI* pPropType = pProp->GetSpecificType();

  switch (pProp->GetCategory())
  {
  case ezPropertyCategory::Member:
    {
      if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags))
      {
        ezStringBuilder sTemp;
        ezReflectionUtils::EnumerationToString(pPropType, pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).ConvertTo<ezInt64>(), sTemp);
        pNode->AddProperty(pProp->GetPropertyName(), sTemp.GetData());
      }
      else if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
      {
        pNode->AddProperty(pProp->GetPropertyName(), pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()));
      }
      else if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
        {
          const ezUuid guid = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).Get<ezUuid>();

          pNode->AddProperty(pProp->GetPropertyName(), guid);
          if (guid.IsValid())
            m_QueuedObjects.Insert(m_pManager->GetObject(guid));
        }
        else
        {
          pNode->AddProperty(pProp->GetPropertyName(), pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()));
        }
      }
      else // ezPropertyFlags::EmbeddedClass
      {
        const ezUuid guid = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).Get<ezUuid>();
        EZ_ASSERT_DEV(guid.IsValid(), "Embedded class cannot be null.");
        pNode->AddProperty(pProp->GetPropertyName(), guid);
        m_QueuedObjects.Insert(m_pManager->GetObject(guid));
      }
    }

    break;

  case ezPropertyCategory::Array:
  case ezPropertyCategory::Set:
    {
      const ezInt32 iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      EZ_ASSERT_DEV(iCount >= 0, "Invalid array property size %i", iCount);

      ezVariantArray values;
      values.SetCount(iCount);

      for (ezInt32 i = 0; i < iCount; ++i)
      {
        values[i] = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i);
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

void ezDocumentObjectConverterWriter::AddProperties(ezAbstractObjectNode* pNode, const ezDocumentObject* pObject)
{
  ezHybridArray<ezAbstractProperty*, 32> Properties;
  pObject->GetTypeAccessor().GetType()->GetAllProperties(Properties);

  for (const auto* pProp : Properties)
  {
    AddProperty(pNode, pProp, pObject);
  }
}

ezAbstractObjectNode* ezDocumentObjectConverterWriter::AddSubObjectToGraph(const ezDocumentObject* pObject, const char* szNodeName)
{
  ezAbstractObjectNode* pNode = m_pGraph->AddNode(pObject->GetGuid(), pObject->GetType()->GetTypeName(), pObject->GetType()->GetTypeVersion(), szNodeName);
  AddProperties(pNode, pObject);
  return pNode;
}



ezDocumentObjectConverterReader::ezDocumentObjectConverterReader(const ezAbstractObjectGraph* pGraph, ezDocumentObjectManager* pManager, Mode mode)
{
  m_pManager = pManager;
  m_pGraph = pGraph;
  m_Mode = mode;
  m_uiUnknownTypeInstances = 0;
}

ezDocumentObject* ezDocumentObjectConverterReader::CreateObjectFromNode(const ezAbstractObjectNode* pNode, ezDocumentObject* pParent, const char* szParentProperty, ezVariant index)
{
  ezDocumentObject* pObject = nullptr;

  switch (m_Mode)
  {
  case ezDocumentObjectConverterReader::Mode::CreateOnly:
    {
      ezRTTI* pType = ezRTTI::FindTypeByName(pNode->GetType());
      if (pType)
      {
        pObject = m_pManager->CreateObject(pType, pNode->GetGuid());
        if (pParent != nullptr)
          pParent->InsertSubObject(pObject, szParentProperty, index);
      }
      else
      {
        ezLog::Error("Cannot create node of unknown type '%s'.", pNode->GetType());
      }
    }
    break;

  case ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument:
    {
      const ezRTTI* pRtti = ezRTTI::FindTypeByName(pNode->GetType());

      if (pRtti)
      {
        pObject = m_pManager->CreateObject(pRtti, pNode->GetGuid());
        m_pManager->AddObject(pObject, pParent, szParentProperty, index);
      }
      else
      {
        if (!m_UnknownTypes.Contains(pNode->GetType()))
        {
          ezLog::Error("Can't create objects of unknown type '%s'", pNode->GetType());
          m_UnknownTypes.Insert(pNode->GetType());
        }

        m_uiUnknownTypeInstances++;
        return nullptr;
      }
    }
    break;
  }
  
  return pObject;
}

void ezDocumentObjectConverterReader::ApplyPropertiesToObject(const ezAbstractObjectNode* pNode, ezDocumentObject* pObject)
{
  //EZ_ASSERT_DEV(pObject->GetChildren().GetCount() == 0, "Can only apply properties to empty objects!");
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

void ezDocumentObjectConverterReader::ApplyDiffToObject(ezObjectAccessorBase* pObjectAccessor, const ezDocumentObject* pObject, ezDeque<ezAbstractGraphDiffOperation>& diff)
{
  ezHybridArray<ezAbstractGraphDiffOperation*, 4> change;
  
  for (auto& op : diff)
  {
    if (op.m_Operation == ezAbstractGraphDiffOperation::Op::PropertyChanged && pObject->GetGuid() == op.m_Node)
      change.PushBack(&op);
  }

  for (auto* op : change)
  {
    ezAbstractProperty* pProp = pObject->GetTypeAccessor().GetType()->FindPropertyByName(op->m_sProperty);
    if (!pProp)
      continue;

    ApplyDiff(pObjectAccessor, pObject, pProp, *op, diff);
  }

  // Recurse into owned sub objects (old or new)
  for (const ezDocumentObject* pSubObject : pObject->GetChildren())
  {
    ApplyDiffToObject(pObjectAccessor, pSubObject, diff);
  }
}

void ezDocumentObjectConverterReader::ApplyDiff(ezObjectAccessorBase* pObjectAccessor, const ezDocumentObject* pObject, ezAbstractProperty* pProp, ezAbstractGraphDiffOperation& op, ezDeque<ezAbstractGraphDiffOperation>& diff)
{
  auto* pObjectMananger = pObject->GetDocumentObjectManager();

  const ezRTTI* pPropType = pProp->GetSpecificType();
  ezStringBuilder sTemp;

  auto NeedsToBeDeleted = [&diff](const ezUuid& guid)->bool
  {
    for (auto& op : diff)
    {
      if (op.m_Operation == ezAbstractGraphDiffOperation::Op::NodeRemoved && guid == op.m_Node)
        return true;
    }
    return false;
  };
  auto NeedsToBeCreated = [&diff](const ezUuid& guid)->ezAbstractGraphDiffOperation*
  {
    for (auto& op : diff)
    {
      if (op.m_Operation == ezAbstractGraphDiffOperation::Op::NodeAdded && guid == op.m_Node)
        return &op;
    }
    return nullptr;
    };

  if (pProp->GetCategory() == ezPropertyCategory::Member)
  {
    if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
    {
      const ezUuid oldGuid = pObjectAccessor->Get<ezUuid>(pObject, pProp);
      const ezUuid newGuid = op.m_Value.Get<ezUuid>();
      if (oldGuid.IsValid())
      {
        if (NeedsToBeDeleted(oldGuid))
        {
          pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(oldGuid));
        }
      }

      if (newGuid.IsValid())
      {
        if (ezAbstractGraphDiffOperation* pCreate = NeedsToBeCreated(newGuid))
        {
          pObjectAccessor->AddObject(pObject, pProp, ezVariant(), ezRTTI::FindTypeByName(pCreate->m_sProperty), pCreate->m_Node);
        }

        const ezDocumentObject* pChild = pObject->GetChild(newGuid);
        EZ_ASSERT_DEV(pChild != nullptr, "References child object does not exist!");
      }
    }
    else if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags | ezPropertyFlags::StandardType | ezPropertyFlags::Pointer))
    {
      pObjectAccessor->SetValue(pObject, pProp, op.m_Value);
    }
    else if (pProp->GetFlags().IsSet(ezPropertyFlags::EmbeddedClass))
    {
      // Noting to do here, value cannot change
    }
  }
  else if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
  {
    const ezVariantArray& values = op.m_Value.Get<ezVariantArray>();
    ezInt32 iCurrentCount = pObjectAccessor->GetCount(pObject, pProp);
    if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType | ezPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
    {
      for (ezUInt32 i = 0; i < values.GetCount(); ++i)
      {
        if (i < (ezUInt32)iCurrentCount)
          pObjectAccessor->SetValue(pObject, pProp, values[i], i);
        else
          pObjectAccessor->InsertValue(pObject, pProp, values[i], i);
      }
      for (ezInt32 i = iCurrentCount - 1; i >= (ezInt32)values.GetCount(); --i)
      {
        pObjectAccessor->RemoveValue(pObject, pProp, i);
      }
    }
    else
    {
      ezInt32 iCurrentCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      {
        ezHybridArray<ezVariant, 16> currentValues;
        pObject->GetTypeAccessor().GetValues(pProp->GetPropertyName(), currentValues);
        for (ezInt32 i = iCurrentCount - 1; i >= 0; --i)
        {
          if (NeedsToBeDeleted(currentValues[i].Get<ezUuid>()))
          {
            pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(currentValues[i].Get<ezUuid>()));
          }
        }

        for (ezUInt32 i = 0; i < values.GetCount(); ++i)
        {
          if (ezAbstractGraphDiffOperation* pCreate = NeedsToBeCreated(values[i].Get<ezUuid>()))
          {
            pObjectAccessor->AddObject(pObject, pProp, i, ezRTTI::FindTypeByName(pCreate->m_sProperty), pCreate->m_Node);
          }
          else
          {
            pObjectAccessor->MoveObject(pObjectAccessor->GetObject(values[i].Get<ezUuid>()), pObject, pProp, i);
          }
        }
      }
    }
  }
}

void ezDocumentObjectConverterReader::ApplyProperty(ezDocumentObject* pObject, ezAbstractProperty* pProp, const ezAbstractObjectNode::Property* pSource)
{
  const ezRTTI* pPropType = pProp->GetSpecificType();
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

        if (pSubObject)
        {
          ApplyPropertiesToObject(pSubNode, pSubObject);
        }
      }
    }
    else if (pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags | ezPropertyFlags::StandardType | ezPropertyFlags::Pointer))
    {
      pObject->GetTypeAccessor().SetValue(pProp->GetPropertyName(), pSource->m_Value);
    }
    else // ezPropertyFlags::EmbeddedClass
    {
      const ezUuid& nodeGuid = pSource->m_Value.Get<ezUuid>();

      const ezUuid subObjectGuid = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).Get<ezUuid>();
      ezDocumentObject* pEmbeddedClassObject = pObject->GetChild(subObjectGuid);
      EZ_ASSERT_DEV(pEmbeddedClassObject != nullptr, "CreateObject should have created all embedded classes!");
      auto* pSubNode = m_pGraph->GetNode(nodeGuid);
      EZ_ASSERT_DEV(pSubNode != nullptr, "invalid document");

      ApplyPropertiesToObject(pSubNode, pEmbeddedClassObject);
    }
  }
  else if (pProp->GetCategory() == ezPropertyCategory::Array || pProp->GetCategory() == ezPropertyCategory::Set)
  {
    const ezVariantArray& array = pSource->m_Value.Get<ezVariantArray>();

    if (pProp->GetFlags().IsAnySet(ezPropertyFlags::StandardType | ezPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
    {
      for (ezUInt32 i = 0; i < array.GetCount(); ++i)
      {
        pObject->GetTypeAccessor().InsertValue(pProp->GetPropertyName(), i, array[i]);
      }
    }
    else
    {
      ezInt32 iCurrentCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      for (ezUInt32 i = 0; i < array.GetCount(); ++i)
      {
        const ezUuid guid = array[i].Get<ezUuid>();

        if (guid.IsValid())
        {
          auto* pSubNode = m_pGraph->GetNode(guid);
          EZ_ASSERT_DEV(pSubNode != nullptr, "invalid document");

          ezDocumentObject* pSubObject = nullptr;
          if (i < (ezUInt32)iCurrentCount)
          {
            // Overwrite existing object
            ezUuid childGuid = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i).ConvertTo<ezUuid>();
            pSubObject = m_pManager->GetObject(childGuid);
          }
          else
          {
            pSubObject = CreateObjectFromNode(pSubNode, pObject, pProp->GetPropertyName(), -1);
          }

          if (pSubObject)
          {
            ApplyPropertiesToObject(pSubNode, pSubObject);
          }
        }
      }
    }
  }
}

