#include <PCH.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <Foundation/IO/MemoryStream.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <Foundation/Serialization/DdlSerializer.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAddObjectCommand, 1, ezRTTIDefaultAllocator<ezAddObjectCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Type", GetType, SetType),
    EZ_MEMBER_PROPERTY("ParentGuid", m_Parent),
    EZ_MEMBER_PROPERTY("ParentProperty", m_sParentProperty),
    EZ_MEMBER_PROPERTY("Index", m_Index),
    EZ_MEMBER_PROPERTY("NewGuid", m_NewObjectGuid),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPasteObjectsCommand, 1, ezRTTIDefaultAllocator<ezPasteObjectsCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ParentGuid", m_Parent),
    EZ_MEMBER_PROPERTY("TextGraph", m_sGraphTextFormat),
    EZ_MEMBER_PROPERTY("Mime", m_sMimeType),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInstantiatePrefabCommand, 1, ezRTTIDefaultAllocator<ezInstantiatePrefabCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ParentGuid", m_Parent),
    EZ_MEMBER_PROPERTY("CreateFromPrefab", m_CreateFromPrefab),
    EZ_MEMBER_PROPERTY("BaseGraph", m_sBasePrefabGraph),
    EZ_MEMBER_PROPERTY("ObjectGraph", m_sObjectGraph),
    EZ_MEMBER_PROPERTY("RemapGuid", m_RemapGuid),
    EZ_MEMBER_PROPERTY("CreatedObjects", m_CreatedRootObject),
    EZ_MEMBER_PROPERTY("AllowPickedPos", m_bAllowPickedPosition),
    EZ_MEMBER_PROPERTY("Index", m_Index),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezUnlinkPrefabCommand, 1, ezRTTIDefaultAllocator<ezUnlinkPrefabCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Object", m_Object),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRemoveObjectCommand, 1, ezRTTIDefaultAllocator<ezRemoveObjectCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMoveObjectCommand, 1, ezRTTIDefaultAllocator<ezMoveObjectCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
    EZ_MEMBER_PROPERTY("NewParentGuid", m_NewParent),
    EZ_MEMBER_PROPERTY("ParentProperty", m_sParentProperty),
    EZ_MEMBER_PROPERTY("Index", m_Index),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSetObjectPropertyCommand, 1, ezRTTIDefaultAllocator<ezSetObjectPropertyCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
    EZ_MEMBER_PROPERTY("NewValue", m_NewValue),
    EZ_MEMBER_PROPERTY("Index", m_Index),
    EZ_MEMBER_PROPERTY("Property", m_sProperty),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezResizeAndSetObjectPropertyCommand, 1, ezRTTIDefaultAllocator<ezResizeAndSetObjectPropertyCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
    EZ_MEMBER_PROPERTY("NewValue", m_NewValue),
    EZ_MEMBER_PROPERTY("Index", m_Index),
    EZ_MEMBER_PROPERTY("Property", m_sProperty),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInsertObjectPropertyCommand, 1, ezRTTIDefaultAllocator<ezInsertObjectPropertyCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
    EZ_MEMBER_PROPERTY("NewValue", m_NewValue),
    EZ_MEMBER_PROPERTY("Index", m_Index),
    EZ_MEMBER_PROPERTY("Property", m_sProperty),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRemoveObjectPropertyCommand, 1, ezRTTIDefaultAllocator<ezRemoveObjectPropertyCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
    EZ_MEMBER_PROPERTY("Index", m_Index),
    EZ_MEMBER_PROPERTY("Property", m_sProperty),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMoveObjectPropertyCommand, 1, ezRTTIDefaultAllocator<ezMoveObjectPropertyCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
    EZ_MEMBER_PROPERTY("OldIndex", m_OldIndex),
    EZ_MEMBER_PROPERTY("NewIndex", m_NewIndex),
    EZ_MEMBER_PROPERTY("Property", m_sProperty),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

////////////////////////////////////////////////////////////////////////
// ezAddObjectCommand
////////////////////////////////////////////////////////////////////////

ezAddObjectCommand::ezAddObjectCommand() :
  m_pType(nullptr), m_pObject(nullptr)
{
}

const char* ezAddObjectCommand::GetType() const
{
  if (m_pType == nullptr)
    return "";

  return m_pType->GetTypeName();
}

void ezAddObjectCommand::SetType(const char* szType)
{
  m_pType = ezRTTI::FindTypeByName(szType);
}

ezStatus ezAddObjectCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (!m_NewObjectGuid.IsValid())
      m_NewObjectGuid.CreateNewUuid();
  }

  ezDocumentObject* pParent = nullptr;
  if (m_Parent.IsValid())
  {
    pParent = pDocument->GetObjectManager()->GetObject(m_Parent);
    if (pParent == nullptr)
      return ezStatus("Add Object: The given parent does not exist!");
  }

  EZ_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanAdd(m_pType, pParent, m_sParentProperty, m_Index));

  if (!bRedo)
  {
    m_pObject = pDocument->GetObjectManager()->CreateObject(m_pType, m_NewObjectGuid);
  }

  pDocument->GetObjectManager()->AddObject(m_pObject, pParent, m_sParentProperty, m_Index);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezAddObjectCommand::UndoInternal(bool bFireEvents)
{
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  ezDocument* pDocument = GetDocument();
  EZ_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(m_pObject));

  pDocument->GetObjectManager()->RemoveObject(m_pObject);
  return ezStatus(EZ_SUCCESS);
}

void ezAddObjectCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    GetDocument()->GetObjectManager()->DestroyObject(m_pObject);
    m_pObject = nullptr;
  }

}


////////////////////////////////////////////////////////////////////////
// ezPasteObjectsCommand
////////////////////////////////////////////////////////////////////////

ezPasteObjectsCommand::ezPasteObjectsCommand()
{
}

ezStatus ezPasteObjectsCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();

  ezDocumentObject* pParent = nullptr;
  if (m_Parent.IsValid())
  {
    pParent = pDocument->GetObjectManager()->GetObject(m_Parent);
    if (pParent == nullptr)
      return ezStatus("Paste Objects: The given parent does not exist!");
  }

  if (!bRedo)
  {
    ezAbstractObjectGraph graph;

    {
      // Deserialize
      ezRawMemoryStreamReader memoryReader(m_sGraphTextFormat.GetData(), m_sGraphTextFormat.GetElementCount());
      ezAbstractGraphDdlSerializer::Read(memoryReader, &graph);
    }

    // Remap
    ezUuid seed;
    seed.CreateNewUuid();
    graph.ReMapNodeGuids(seed);

    ezDocumentObjectConverterReader reader(&graph, pDocument->GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateOnly);

    ezHybridArray<ezDocument::PasteInfo, 16> ToBePasted;

    auto& nodes = graph.GetAllNodes();
    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      auto* pNode = it.Value();
      if (ezStringUtils::IsEqual(pNode->GetNodeName(), "root"))
      {
        auto* pNewObject = reader.CreateObjectFromNode(pNode);

        if (pNewObject)
        {
          reader.ApplyPropertiesToObject(pNode, pNewObject);

          auto& ref = ToBePasted.ExpandAndGetRef();
          ref.m_pObject = pNewObject;
          ref.m_pParent = pParent;
        }
      }
    }

    if (pDocument->Paste(ToBePasted, graph, true, m_sMimeType))
    {
      for (const auto& item : ToBePasted)
      {
        auto& po = m_PastedObjects.ExpandAndGetRef();
        po.m_pObject = item.m_pObject;
        po.m_pParent = item.m_pParent;
        po.m_Index = item.m_pObject->GetPropertyIndex();
        po.m_sParentProperty = item.m_pObject->GetParentProperty();
      }
    }
    else
    {
      for (const auto& item : ToBePasted)
      {
        pDocument->GetObjectManager()->DestroyObject(item.m_pObject);
      }
    }

    if (m_PastedObjects.IsEmpty())
      return ezStatus("Paste Objects: nothing was pasted!");
  }
  else
  {
    // Re-add at recorded place.
    for (auto& po : m_PastedObjects)
    {
      pDocument->GetObjectManager()->AddObject(po.m_pObject, po.m_pParent, po.m_sParentProperty, po.m_Index);
    }
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezPasteObjectsCommand::UndoInternal(bool bFireEvents)
{
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  ezDocument* pDocument = GetDocument();

  for (auto& po : m_PastedObjects)
  {
    EZ_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(po.m_pObject));

    pDocument->GetObjectManager()->RemoveObject(po.m_pObject);
  }

  return ezStatus(EZ_SUCCESS);
}

void ezPasteObjectsCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    for (auto& po : m_PastedObjects)
    {
      GetDocument()->GetObjectManager()->DestroyObject(po.m_pObject);
    }
    m_PastedObjects.Clear();
  }

}

////////////////////////////////////////////////////////////////////////
// ezInstantiatePrefabCommand
////////////////////////////////////////////////////////////////////////

ezInstantiatePrefabCommand::ezInstantiatePrefabCommand()
{
  m_bAllowPickedPosition = true;
}

ezStatus ezInstantiatePrefabCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();

  ezDocumentObject* pParent = nullptr;
  if (m_Parent.IsValid())
  {
    pParent = pDocument->GetObjectManager()->GetObject(m_Parent);
    if (pParent == nullptr)
      return ezStatus("Instantiate Prefab: The given parent does not exist!");
  }

  if (!bRedo)
  {
    // TODO: this is hard-coded, it only works for scene documents !
    const ezRTTI* pRootObjectType = ezRTTI::FindTypeByName("ezGameObject");
    const char* szParentProperty = "Children";

    ezDocumentObject* pRootObject = nullptr;
    ezHybridArray<ezDocument::PasteInfo, 16> ToBePasted;
    ezAbstractObjectGraph graph;

    // create root object
    {
      EZ_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanAdd(pRootObjectType, pParent, szParentProperty, m_Index));

      // use the same GUID for the root object ID as the remap GUID, this way the object ID is deterministic and reproducible
      m_CreatedRootObject = m_RemapGuid;

      pRootObject = pDocument->GetObjectManager()->CreateObject(pRootObjectType, m_CreatedRootObject);

      auto& ref = ToBePasted.ExpandAndGetRef();
      ref.m_pObject = pRootObject;
      ref.m_pParent = pParent;
      ref.m_Index = m_Index;
    }

    // update meta data
    // this is read when Paste is executed, to determine a good node name
    {
      // if prefabs are not allowed in this document, just create this as a regular object, with no link to the prefab template
      if (pDocument->ArePrefabsAllowed())
      {
        auto pMeta = pDocument->m_DocumentObjectMetaData.BeginModifyMetaData(m_CreatedRootObject);
        pMeta->m_CreateFromPrefab = m_CreateFromPrefab;
        pMeta->m_PrefabSeedGuid = m_RemapGuid;
        pMeta->m_sBasePrefab = m_sBasePrefabGraph;
        pDocument->m_DocumentObjectMetaData.EndModifyMetaData(ezDocumentObjectMetaData::PrefabFlag);
      }
      else
      {
        pDocument->ShowDocumentStatus("Nested prefabs are not allowed. Instantiated object will not be linked to prefab template.");
      }
    }

    if (pDocument->Paste(ToBePasted, graph, m_bAllowPickedPosition, "application/ezEditor.ezAbstractGraph"))
    {
      for (const auto& item : ToBePasted)
      {
        auto& po = m_PastedObjects.ExpandAndGetRef();
        po.m_pObject = item.m_pObject;
        po.m_pParent = item.m_pParent;
        po.m_Index = item.m_pObject->GetPropertyIndex();
        po.m_sParentProperty = item.m_pObject->GetParentProperty();
      }
    }
    else
    {
      for (const auto& item : ToBePasted)
      {
        pDocument->GetObjectManager()->DestroyObject(item.m_pObject);
      }

      ToBePasted.Clear();
    }

    if (m_PastedObjects.IsEmpty())
      return ezStatus("Paste Objects: nothing was pasted!");

    if (!m_sObjectGraph.IsEmpty())
      ezPrefabUtils::LoadGraph(graph, m_sObjectGraph);
    else
      ezPrefabUtils::LoadGraph(graph, m_sBasePrefabGraph);

    graph.ReMapNodeGuids(m_RemapGuid);

    // a prefab can have multiple top level nodes
    ezHybridArray<ezAbstractObjectNode*, 4> rootNodes;
    ezPrefabUtils::GetRootNodes(graph, rootNodes);

    for (auto* pPrefabRoot : rootNodes)
    {
      ezDocumentObjectConverterReader reader(&graph, pDocument->GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateOnly);

      if (auto* pNewObject = reader.CreateObjectFromNode(pPrefabRoot))
      {
        reader.ApplyPropertiesToObject(pPrefabRoot, pNewObject);

        // attach all prefab nodes to the main group node
        pDocument->GetObjectManager()->AddObject(pNewObject, pRootObject, szParentProperty, -1);
      }
    }
  }
  else
  {
    // Re-add at recorded place.
    for (auto& po : m_PastedObjects)
    {
      pDocument->GetObjectManager()->AddObject(po.m_pObject, po.m_pParent, po.m_sParentProperty, po.m_Index);
    }
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezInstantiatePrefabCommand::UndoInternal(bool bFireEvents)
{
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  ezDocument* pDocument = GetDocument();

  for (auto& po : m_PastedObjects)
  {
    EZ_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(po.m_pObject));

    pDocument->GetObjectManager()->RemoveObject(po.m_pObject);
  }

  return ezStatus(EZ_SUCCESS);
}

void ezInstantiatePrefabCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    for (auto& po : m_PastedObjects)
    {
      GetDocument()->GetObjectManager()->DestroyObject(po.m_pObject);
    }
    m_PastedObjects.Clear();
  }

}


//////////////////////////////////////////////////////////////////////////
// ezUnlinkPrefabCommand
//////////////////////////////////////////////////////////////////////////

ezStatus ezUnlinkPrefabCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();
  ezDocumentObject* pObject = pDocument->GetObjectManager()->GetObject(m_Object);

  if (pObject == nullptr)
    return ezStatus("Unlink Prefab: The given object does not exist!");

  // store previous values
  if (!bRedo)
  {
    auto pMeta = pDocument->m_DocumentObjectMetaData.BeginReadMetaData(m_Object);
    m_OldCreateFromPrefab = pMeta->m_CreateFromPrefab;
    m_OldRemapGuid = pMeta->m_PrefabSeedGuid;
    m_sOldGraphTextFormat = pMeta->m_sBasePrefab;
    pDocument->m_DocumentObjectMetaData.EndReadMetaData();
  }

  // unlink
  {
    auto pMeta = pDocument->m_DocumentObjectMetaData.BeginModifyMetaData(m_Object);
    pMeta->m_CreateFromPrefab = ezUuid();
    pMeta->m_PrefabSeedGuid = ezUuid();
    pMeta->m_sBasePrefab.Clear();
    pDocument->m_DocumentObjectMetaData.EndModifyMetaData(ezDocumentObjectMetaData::PrefabFlag);
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezUnlinkPrefabCommand::UndoInternal(bool bFireEvents)
{
  ezDocument* pDocument = GetDocument();
  ezDocumentObject* pObject = pDocument->GetObjectManager()->GetObject(m_Object);

  if (pObject == nullptr)
    return ezStatus("Unlink Prefab: The given object does not exist!");

  // restore link
  {
    auto pMeta = pDocument->m_DocumentObjectMetaData.BeginModifyMetaData(m_Object);
    pMeta->m_CreateFromPrefab = m_OldCreateFromPrefab;
    pMeta->m_PrefabSeedGuid = m_OldRemapGuid;
    pMeta->m_sBasePrefab = m_sOldGraphTextFormat;
    pDocument->m_DocumentObjectMetaData.EndModifyMetaData(ezDocumentObjectMetaData::PrefabFlag);
  }

  return ezStatus(EZ_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// ezRemoveObjectCommand
////////////////////////////////////////////////////////////////////////

ezRemoveObjectCommand::ezRemoveObjectCommand() :
  m_pParent(nullptr),
  m_pObject(nullptr)
{
}

ezStatus ezRemoveObjectCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return ezStatus("Remove Object: The given object does not exist!");
    }
    else
      return ezStatus("Remove Object: The given object does not exist!");

    EZ_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(m_pObject));

    m_pParent = const_cast<ezDocumentObject*>(m_pObject->GetParent());
    m_sParentProperty = m_pObject->GetParentProperty();
    const ezIReflectedTypeAccessor& accessor = m_pObject->GetParent()->GetTypeAccessor();
    m_Index = accessor.GetPropertyChildIndex(m_pObject->GetParentProperty(), m_pObject->GetGuid());
  }

  pDocument->GetObjectManager()->RemoveObject(m_pObject);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezRemoveObjectCommand::UndoInternal(bool bFireEvents)
{
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  ezDocument* pDocument = GetDocument();
  EZ_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanAdd(m_pObject->GetTypeAccessor().GetType(), m_pParent, m_sParentProperty, m_Index));

  pDocument->GetObjectManager()->AddObject(m_pObject, m_pParent, m_sParentProperty, m_Index);
  return ezStatus(EZ_SUCCESS);
}

void ezRemoveObjectCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasDone)
  {
    GetDocument()->GetObjectManager()->DestroyObject(m_pObject);
    m_pObject = nullptr;
  }
}


////////////////////////////////////////////////////////////////////////
// ezMoveObjectCommand
////////////////////////////////////////////////////////////////////////

ezMoveObjectCommand::ezMoveObjectCommand()
{
  m_pObject = nullptr;
  m_pOldParent = nullptr;
  m_pNewParent = nullptr;
}

ezStatus ezMoveObjectCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return ezStatus("Move Object: The given object does not exist!");
    }

    if (m_NewParent.IsValid())
    {
      m_pNewParent = pDocument->GetObjectManager()->GetObject(m_NewParent);
      if (m_pNewParent == nullptr)
        return ezStatus("Move Object: The new parent does not exist!");
    }

    m_pOldParent = const_cast<ezDocumentObject*>(m_pObject->GetParent());
    m_sOldParentProperty = m_pObject->GetParentProperty();
    const ezIReflectedTypeAccessor& accessor = m_pOldParent->GetTypeAccessor();
    m_OldIndex = accessor.GetPropertyChildIndex(m_pObject->GetParentProperty(), m_pObject->GetGuid());

    EZ_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanMove(m_pObject, m_pNewParent, m_sParentProperty, m_Index));
  }

  pDocument->GetObjectManager()->MoveObject(m_pObject, m_pNewParent, m_sParentProperty, m_Index);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezMoveObjectCommand::UndoInternal(bool bFireEvents)
{
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  ezDocument* pDocument = GetDocument();

  ezVariant FinalOldPosition = m_OldIndex;

  if (m_Index.CanConvertTo<ezInt32>() && m_pOldParent == m_pNewParent)
  {
    // If we are moving an object downwards, we must move by more than 1 (+1 would be behind the same object, which is still the same position)
    // so an object must always be moved by at least +2
    // moving UP can be done by -1, so when we undo that, we must ensure to move +2

    ezInt32 iNew = m_Index.ConvertTo<ezInt32>();
    ezInt32 iOld = m_OldIndex.ConvertTo<ezInt32>();

    if (iNew < iOld)
    {
      FinalOldPosition = iOld + 1;
    }
  }

  EZ_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanMove(m_pObject, m_pOldParent, m_sOldParentProperty, FinalOldPosition));

  pDocument->GetObjectManager()->MoveObject(m_pObject, m_pOldParent, m_sOldParentProperty, FinalOldPosition);

  return ezStatus(EZ_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// ezSetObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

ezSetObjectPropertyCommand::ezSetObjectPropertyCommand()
{
  m_pObject = nullptr;
}

ezStatus ezSetObjectPropertyCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return ezStatus("Set Property: The given object does not exist!");
    }
    else
      return ezStatus("Set Property: The given object does not exist!");

    ezIReflectedTypeAccessor& accessor0 = m_pObject->GetTypeAccessor();

    m_OldValue = accessor0.GetValue(m_sProperty, m_Index);
    ezAbstractProperty* pProp = accessor0.GetType()->FindPropertyByName(m_sProperty);
    if (pProp == nullptr || !m_OldValue.IsValid())
      return ezStatus(ezFmt("Set Property: The property '{0}' does not exist", m_sProperty));

    if (pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner))
    {
      return ezStatus(ezFmt("Set Property: The property '{0}' is a PointerOwner, use ezAddObjectCommand instead", m_sProperty));
    }
  }

  return pDocument->GetObjectManager()->SetValue(m_pObject, m_sProperty, m_NewValue, m_Index);
}

ezStatus ezSetObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  if (bFireEvents)
  {
    return GetDocument()->GetObjectManager()->SetValue(m_pObject, m_sProperty, m_OldValue, m_Index);
  }
  else
  {
    ezIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    if (!accessor.SetValue(m_sProperty, m_OldValue, m_Index))
    {
      return ezStatus(ezFmt("Set Property: The property '{0}' does not exist", m_sProperty));
    }
  }
  return ezStatus(EZ_SUCCESS);
}

////////////////////////////////////////////////////////////////////////
// ezSetObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

ezResizeAndSetObjectPropertyCommand::ezResizeAndSetObjectPropertyCommand()
{
  m_pObject = nullptr;
}

ezStatus ezResizeAndSetObjectPropertyCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return ezStatus("Set Property: The given object does not exist!");
    }
    else
      return ezStatus("Set Property: The given object does not exist!");

    const ezInt32 uiIndex = m_Index.ConvertTo<ezInt32>();

    ezIReflectedTypeAccessor& accessor0 = m_pObject->GetTypeAccessor();

    const ezInt32 iCount = accessor0.GetCount(m_sProperty);

    for (ezInt32 i = iCount; i <= uiIndex; ++i)
    {
      ezInsertObjectPropertyCommand ins;
      ins.m_Object = m_Object;
      ins.m_sProperty = m_sProperty;
      ins.m_Index = i;
      ins.m_NewValue = ezToolsReflectionUtils::GetDefaultVariantFromType(m_NewValue.GetType());

      AddSubCommand(ins);
    }

    ezSetObjectPropertyCommand set;
    set.m_sProperty = m_sProperty;
    set.m_Index = m_Index;
    set.m_NewValue = m_NewValue;
    set.m_Object = m_Object;

    AddSubCommand(set);
  }

  return ezStatus(EZ_SUCCESS);
}

////////////////////////////////////////////////////////////////////////
// ezInsertObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

ezInsertObjectPropertyCommand::ezInsertObjectPropertyCommand()
{
  m_pObject = nullptr;
}

ezStatus ezInsertObjectPropertyCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return ezStatus("Insert Property: The given object does not exist!");
    }
    else
      return ezStatus("Insert Property: The given object does not exist!");

    if (m_Index.CanConvertTo<ezInt32>() && m_Index.ConvertTo<ezInt32>() == -1)
    {
      ezIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
      m_Index = accessor.GetCount(m_sProperty.GetData());
    }
  }

  return pDocument->GetObjectManager()->InsertValue(m_pObject, m_sProperty, m_NewValue, m_Index);
}

ezStatus ezInsertObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  if (bFireEvents)
  {
    return GetDocument()->GetObjectManager()->RemoveValue(m_pObject, m_sProperty, m_Index);
  }
  else
  {
    ezIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    if (!accessor.RemoveValue(m_sProperty, m_Index))
    {
      return ezStatus(ezFmt("Insert Property: The property '{0}' does not exist", m_sProperty));
    }
  }

  return ezStatus(EZ_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// ezRemoveObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

ezRemoveObjectPropertyCommand::ezRemoveObjectPropertyCommand()
{
  m_pObject = nullptr;
}

ezStatus ezRemoveObjectPropertyCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return ezStatus("Remove Property: The given object does not exist!");
    }
    else
      return ezStatus("Remove Property: The given object does not exist!");
  }

  return pDocument->GetObjectManager()->RemoveValue(m_pObject, m_sProperty, m_Index);
}

ezStatus ezRemoveObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  if (bFireEvents)
  {
    return GetDocument()->GetObjectManager()->InsertValue(m_pObject, m_sProperty, m_OldValue, m_Index);
  }
  else
  {
    ezIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    if (!accessor.InsertValue(m_sProperty, m_Index, m_OldValue))
    {
      return ezStatus(ezFmt("Remove Property: Undo failed! The index '{0}' in property '{1}' does not exist", m_Index.ConvertTo<ezString>(), m_sProperty));
    }
  }
  return ezStatus(EZ_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// ezMoveObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

ezMoveObjectPropertyCommand::ezMoveObjectPropertyCommand()
{
  m_pObject = nullptr;
}

ezStatus ezMoveObjectPropertyCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
    if (m_pObject == nullptr)
      return ezStatus("Move Property: The given object does not exist.");
  }

  return GetDocument()->GetObjectManager()->MoveValue(m_pObject, m_sProperty, m_OldIndex, m_NewIndex);
}

ezStatus ezMoveObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  ezVariant FinalOldPosition = m_OldIndex;
  ezVariant FinalNewPosition = m_NewIndex;

  if (m_OldIndex.CanConvertTo<ezInt32>())
  {
    // If we are moving an object downwards, we must move by more than 1 (+1 would be behind the same object, which is still the same position)
    // so an object must always be moved by at least +2
    // moving UP can be done by -1, so when we undo that, we must ensure to move +2

    ezInt32 iNew = m_NewIndex.ConvertTo<ezInt32>();
    ezInt32 iOld = m_OldIndex.ConvertTo<ezInt32>();

    if (iNew < iOld)
    {
      FinalOldPosition = iOld + 1;
    }

    // The new position is relative to the original array, so we need to substract one to account for
    // the removal of the same element at the lower index.
    if (iNew > iOld)
    {
      FinalNewPosition = iNew - 1;
    }
  }

  return GetDocument()->GetObjectManager()->MoveValue(m_pObject, m_sProperty, FinalOldPosition, FinalNewPosition);
}
