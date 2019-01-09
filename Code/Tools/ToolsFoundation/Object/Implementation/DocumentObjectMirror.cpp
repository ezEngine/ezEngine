#include <PCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezObjectChange, ezNoBase, 1, ezRTTIDefaultAllocator<ezObjectChange>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Change", m_Change),
    EZ_MEMBER_PROPERTY("Root", m_Root),
    EZ_ARRAY_MEMBER_PROPERTY("Steps", m_Steps),
    EZ_MEMBER_PROPERTY("Graph", m_GraphData),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezObjectChange::ezObjectChange(const ezObjectChange&)
{
  EZ_REPORT_FAILURE("Not supported!");
}

void ezObjectChange::GetGraph(ezAbstractObjectGraph& graph) const
{
  graph.Clear();

  ezRawMemoryStreamReader reader(m_GraphData);
  ezAbstractGraphBinarySerializer::Read(reader, &graph);
}

void ezObjectChange::SetGraph(ezAbstractObjectGraph& graph)
{
  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezAbstractGraphBinarySerializer::Write(writer, &graph);

  m_GraphData = ezArrayPtr<const ezUInt8>(storage.GetData(), storage.GetStorageSize());
}

ezObjectChange::ezObjectChange(ezObjectChange&& rhs)
{
  m_Change = std::move(rhs.m_Change);
  m_Root = rhs.m_Root;
  m_Steps = std::move(rhs.m_Steps);
  m_GraphData = std::move(rhs.m_GraphData);
}

void ezObjectChange::operator=(ezObjectChange&& rhs)
{
  m_Change = std::move(rhs.m_Change);
  m_Root = rhs.m_Root;
  m_Steps = std::move(rhs.m_Steps);
  m_GraphData = std::move(rhs.m_GraphData);
}

void ezObjectChange::operator=(ezObjectChange& rhs)
{
  EZ_REPORT_FAILURE("Not supported!");
}


ezDocumentObjectMirror::ezDocumentObjectMirror()
{
  m_pContext = nullptr;
  m_pManager = nullptr;
}

ezDocumentObjectMirror::~ezDocumentObjectMirror()
{
  EZ_ASSERT_DEV(m_pManager == nullptr && m_pContext == nullptr, "Need to call DeInit before d-tor!");
}

void ezDocumentObjectMirror::InitSender(const ezDocumentObjectManager* pManager)
{
  m_pManager = pManager;
  m_pManager->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezDocumentObjectMirror::TreeStructureEventHandler, this));
  m_pManager->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezDocumentObjectMirror::TreePropertyEventHandler, this));
}

void ezDocumentObjectMirror::InitReceiver(ezRttiConverterContext* pContext)
{
  m_pContext = pContext;
}

void ezDocumentObjectMirror::DeInit()
{
  if (m_pManager)
  {
    m_pManager->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezDocumentObjectMirror::TreeStructureEventHandler, this));
    m_pManager->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezDocumentObjectMirror::TreePropertyEventHandler, this));
    m_pManager = nullptr;
  }

  if (m_pContext)
  {
    m_pContext = nullptr;
  }
}

void ezDocumentObjectMirror::SetFilterFunction(FilterFunction filter)
{
  m_Filter = filter;
}

void ezDocumentObjectMirror::SendDocument()
{
  const auto* pRoot = m_pManager->GetRootObject();
  for (auto* pChild : pRoot->GetChildren())
  {
    if (IsDiscardedByFilter(pRoot, pChild->GetParentProperty()))
      continue;

    ezObjectChange change;
    change.m_Change.m_Operation = ezObjectChangeType::NodeAdded;
    change.m_Change.m_Value = pChild->GetGuid();

    ezAbstractObjectGraph graph;
    ezDocumentObjectConverterWriter objectConverter(&graph, m_pManager);
    objectConverter.AddObjectToGraph(pChild, "Object");
    change.SetGraph(graph);

    ApplyOp(change);
  }
}

void ezDocumentObjectMirror::Clear()
{
  if (m_pManager)
  {
    const auto* pRoot = m_pManager->GetRootObject();
    for (auto* pChild : pRoot->GetChildren())
    {
      ezObjectChange change;
      change.m_Change.m_Operation = ezObjectChangeType::NodeRemoved;
      change.m_Change.m_Value = pChild->GetGuid();

      /*ezAbstractObjectGraph graph;
      ezDocumentObjectConverterWriter objectConverter(&graph, m_pManager);
      ezAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(pChild, "Object");
      change.SetGraph(graph);*/

      ApplyOp(change);
    }
  }

  if (m_pContext)
  {
    m_pContext->Clear();
  }
}

void ezDocumentObjectMirror::TreeStructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  if (e.m_pNewParent && IsDiscardedByFilter(e.m_pNewParent, e.m_sParentProperty))
    return;
  if (e.m_pPreviousParent && IsDiscardedByFilter(e.m_pPreviousParent, e.m_sParentProperty))
    return;

  switch (e.m_EventType)
  {
    case ezDocumentObjectStructureEvent::Type::AfterObjectMoved:
    {
      if (IsHeapAllocated(e.m_pNewParent, e.m_sParentProperty))
      {
        if (e.m_pNewParent == nullptr || e.m_pNewParent == m_pManager->GetRootObject())
        {
          // Object is now a root object, nothing to do to attach it to its new parent.
          break;
        }

        if (e.GetProperty()->GetCategory() == ezPropertyCategory::Set && e.m_pPreviousParent == e.m_pNewParent)
        {
          // Sets only have ordering in the editor. We can ignore set order changes in the mirror.
          break;
        }
        ezObjectChange change;
        CreatePath(change, e.m_pNewParent, e.m_sParentProperty);

        change.m_Change.m_Operation = ezObjectChangeType::PropertyInserted;
        change.m_Change.m_Index = e.getInsertIndex();
        change.m_Change.m_Value = e.m_pObject->GetGuid();

        ApplyOp(change);
        break;
      }
      // Intended falltrough as non ptr object might as well be destroyed and rebuild.
    }
      // case ezDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      ezObjectChange change;
      CreatePath(change, e.m_pNewParent, e.m_sParentProperty);

      change.m_Change.m_Operation = ezObjectChangeType::NodeAdded;
      change.m_Change.m_Index = e.getInsertIndex();
      change.m_Change.m_Value = e.m_pObject->GetGuid();

      ezAbstractObjectGraph graph;
      ezDocumentObjectConverterWriter objectConverter(&graph, m_pManager);
      objectConverter.AddObjectToGraph(e.m_pObject, "Object");
      change.SetGraph(graph);

      ApplyOp(change);
    }
    break;
    case ezDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    {
      if (IsHeapAllocated(e.m_pPreviousParent, e.m_sParentProperty))
      {
        EZ_ASSERT_DEBUG(IsHeapAllocated(e.m_pNewParent, e.m_sParentProperty),
                        "Old and new parent must have the same heap allocation state!");
        if (e.m_pPreviousParent == nullptr || e.m_pPreviousParent == m_pManager->GetRootObject())
        {
          // Object is currently a root object, nothing to do to detach it from its parent.
          break;
        }

        if (e.GetProperty()->GetCategory() == ezPropertyCategory::Set && e.m_pPreviousParent == e.m_pNewParent)
        {
          // Sets only have ordering in the editor. We can ignore set order changes in the mirror.
          break;
        }

        ezObjectChange change;
        CreatePath(change, e.m_pPreviousParent, e.m_sParentProperty);

        // Do not delete heap object, just remove it from its owner.
        change.m_Change.m_Operation = ezObjectChangeType::PropertyRemoved;
        change.m_Change.m_Index = e.m_OldPropertyIndex;
        change.m_Change.m_Value = e.m_pObject->GetGuid();

        ApplyOp(change);
        break;
      }
      else
      {
        ezObjectChange change;
        CreatePath(change, e.m_pPreviousParent, e.m_sParentProperty);

        change.m_Change.m_Operation = ezObjectChangeType::PropertyRemoved;
        change.m_Change.m_Index = e.m_OldPropertyIndex;
        change.m_Change.m_Value = e.m_pObject->GetGuid();

        ApplyOp(change);
        break;
      }
    }
      // case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    case ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      ezObjectChange change;
      CreatePath(change, e.m_pPreviousParent, e.m_sParentProperty);

      change.m_Change.m_Operation = ezObjectChangeType::NodeRemoved;
      change.m_Change.m_Index = e.m_OldPropertyIndex;
      change.m_Change.m_Value = e.m_pObject->GetGuid();

      ApplyOp(change);
    }
    break;

    default:
      break;
  }
}

void ezDocumentObjectMirror::TreePropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (IsDiscardedByFilter(e.m_pObject, e.m_sProperty))
    return;

  switch (e.m_EventType)
  {
    case ezDocumentObjectPropertyEvent::Type::PropertySet:
    {
      ezObjectChange change;
      CreatePath(change, e.m_pObject, e.m_sProperty);

      change.m_Change.m_Operation = ezObjectChangeType::PropertySet;
      change.m_Change.m_Index = e.m_NewIndex;
      change.m_Change.m_Value = e.m_NewValue;
      ApplyOp(change);
    }
    break;
    case ezDocumentObjectPropertyEvent::Type::PropertyInserted:
    {
      ezObjectChange change;
      CreatePath(change, e.m_pObject, e.m_sProperty);

      change.m_Change.m_Operation = ezObjectChangeType::PropertyInserted;
      change.m_Change.m_Index = e.m_NewIndex;
      change.m_Change.m_Value = e.m_NewValue;
      ApplyOp(change);
    }
    break;
    case ezDocumentObjectPropertyEvent::Type::PropertyRemoved:
    {
      ezObjectChange change;
      CreatePath(change, e.m_pObject, e.m_sProperty);

      change.m_Change.m_Operation = ezObjectChangeType::PropertyRemoved;
      change.m_Change.m_Index = e.m_OldIndex;
      change.m_Change.m_Value = e.m_OldValue;
      ApplyOp(change);
    }
    break;
    case ezDocumentObjectPropertyEvent::Type::PropertyMoved:
    {
      ezUInt32 uiOldIndex = e.m_OldIndex.ConvertTo<ezUInt32>();
      ezUInt32 uiNewIndex = e.m_NewIndex.ConvertTo<ezUInt32>();
      EZ_ASSERT_DEBUG(e.m_NewValue.IsValid(), "Value must be valid");

      {
        ezObjectChange change;
        CreatePath(change, e.m_pObject, e.m_sProperty);

        change.m_Change.m_Operation = ezObjectChangeType::PropertyRemoved;
        change.m_Change.m_Index = uiOldIndex;
        change.m_Change.m_Value = e.m_NewValue;
        ApplyOp(change);
      }

      if (uiNewIndex > uiOldIndex)
      {
        uiNewIndex -= 1;
      }

      {
        ezObjectChange change;
        CreatePath(change, e.m_pObject, e.m_sProperty);

        change.m_Change.m_Operation = ezObjectChangeType::PropertyInserted;
        change.m_Change.m_Index = uiNewIndex;
        change.m_Change.m_Value = e.m_NewValue;
        ApplyOp(change);
      }

      return;
    }
    break;
  }
}

void* ezDocumentObjectMirror::GetNativeObjectPointer(const ezDocumentObject* pObject)
{
  auto object = m_pContext->GetObjectByGUID(pObject->GetGuid());
  return object.m_pObject;
}

const void* ezDocumentObjectMirror::GetNativeObjectPointer(const ezDocumentObject* pObject) const
{
  auto object = m_pContext->GetObjectByGUID(pObject->GetGuid());
  return object.m_pObject;
}

bool ezDocumentObjectMirror::IsRootObject(const ezDocumentObject* pParent)
{
  return (pParent == nullptr || pParent == m_pManager->GetRootObject());
}

bool ezDocumentObjectMirror::IsHeapAllocated(const ezDocumentObject* pParent, const char* szParentProperty)
{
  if (pParent == nullptr || pParent == m_pManager->GetRootObject())
    return true;

  const ezRTTI* pRtti = pParent->GetTypeAccessor().GetType();

  auto* pProp = pRtti->FindPropertyByName(szParentProperty);
  return pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner);
}


bool ezDocumentObjectMirror::IsDiscardedByFilter(const ezDocumentObject* pObject, const char* szProperty) const
{
  if (m_Filter.IsValid())
  {
    return !m_Filter(pObject, szProperty);
  }
  return false;
}

void ezDocumentObjectMirror::CreatePath(ezObjectChange& out_change, const ezDocumentObject* pRoot, const char* szProperty)
{
  if (pRoot && pRoot->GetDocumentObjectManager()->GetRootObject() != pRoot)
  {
    ezHybridArray<const ezDocumentObject*, 8> path;
    out_change.m_Root = FindRootOpObject(pRoot, path);
    FlattenSteps(path, out_change.m_Steps);
  }

  out_change.m_Change.m_sProperty = szProperty;
}

ezUuid ezDocumentObjectMirror::FindRootOpObject(const ezDocumentObject* pParent, ezHybridArray<const ezDocumentObject*, 8>& path)
{
  path.PushBack(pParent);

  if (!pParent->IsOnHeap())
  {
    return FindRootOpObject(pParent->GetParent(), path);
  }
  else
  {
    return pParent->GetGuid();
  }
}

void ezDocumentObjectMirror::FlattenSteps(const ezArrayPtr<const ezDocumentObject* const> path,
                                          ezHybridArray<ezPropertyPathStep, 2>& out_steps)
{
  ezUInt32 uiCount = path.GetCount();
  EZ_ASSERT_DEV(uiCount > 0, "Path must not be empty!");
  EZ_ASSERT_DEV(path[uiCount - 1]->IsOnHeap(), "Root of steps must be on heap!");

  // Only root object? Then there is no path from it.
  if (uiCount == 1)
    return;

  for (ezInt32 i = (ezInt32)uiCount - 2; i >= 0; --i)
  {
    const ezDocumentObject* pObject = path[i];
    out_steps.PushBack(ezPropertyPathStep({pObject->GetParentProperty(), pObject->GetPropertyIndex()}));
  }
}

void ezDocumentObjectMirror::ApplyOp(ezObjectChange& change)
{
  ezRttiConverterObject object;
  if (change.m_Root.IsValid())
  {
    object = m_pContext->GetObjectByGUID(change.m_Root);
    if (!object.m_pObject)
      return;
    // EZ_ASSERT_DEV(object.m_pObject != nullptr, "Root object does not exist in mirrored native object!");
  }

  ezPropertyPath propPath;
  if (propPath.InitializeFromPath(*object.m_pType, change.m_Steps).Failed())
  {
    ezLog::Error("Failed to init property path on object of type '{0}'.", object.m_pType->GetTypeName());
    return;
  }
  propPath.WriteToLeafObject(object.m_pObject, *object.m_pType, [this, &change](void* pLeaf, const ezRTTI& pType)
  {
    ApplyOp(ezRttiConverterObject(&pType, pLeaf), change);
  });
}

void ezDocumentObjectMirror::ApplyOp(ezRttiConverterObject object, const ezObjectChange& change)
{
  ezAbstractProperty* pProp = nullptr;

  if (object.m_pType != nullptr)
  {
    pProp = object.m_pType->FindPropertyByName(change.m_Change.m_sProperty);
    if (pProp == nullptr)
    {
      ezLog::Error("Property '{0}' not found, can't apply mirror op!", change.m_Change.m_sProperty);
      return;
    }
  }

  switch (change.m_Change.m_Operation)
  {
    case ezObjectChangeType::NodeAdded:
    {
      ezAbstractObjectGraph graph;
      change.GetGraph(graph);
      ezRttiConverterReader reader(&graph, m_pContext);
      const ezAbstractObjectNode* pNode = graph.GetNodeByName("Object");

      void* pValue = reader.CreateObjectFromNode(pNode);
      if (!pValue)
      {
        // Can't create object, exiting.
        return;
      }

      if (!change.m_Root.IsValid())
      {
        // Create without parent (root element)
        return;
      }

      if (pProp->GetCategory() == ezPropertyCategory::Member)
      {
        auto pSpecificProp = static_cast<ezAbstractMemberProperty*>(pProp);
        if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
        {
          pSpecificProp->SetValuePtr(object.m_pObject, &pValue);
        }
        else
        {
          pSpecificProp->SetValuePtr(object.m_pObject, pValue);
        }
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<ezAbstractArrayProperty*>(pProp);
        if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.ConvertTo<ezUInt32>(), &pValue);
        }
        else
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.ConvertTo<ezUInt32>(), pValue);
        }
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Set)
      {
        EZ_ASSERT_DEV(pProp->GetFlags().IsSet(ezPropertyFlags::Pointer), "Set object must always be pointers!");
        auto pSpecificProp = static_cast<ezAbstractSetProperty*>(pProp);
        ezReflectionUtils::InsertSetPropertyValue(pSpecificProp, object.m_pObject, ezVariant(pValue));
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<ezAbstractMapProperty*>(pProp);
        if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.Get<ezString>(), &pValue);
        }
        else
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.Get<ezString>(), pValue);
        }
      }

      if (!pProp->GetFlags().AreAllSet(ezPropertyFlags::Pointer | ezPropertyFlags::PointerOwner))
      {
        m_pContext->DeleteObject(pNode->GetGuid());
      }
    }
    break;
    case ezObjectChangeType::NodeRemoved:
    {
      if (!change.m_Root.IsValid())
      {
        // Delete root object
        m_pContext->DeleteObject(change.m_Change.m_Value.Get<ezUuid>());
        return;
      }

      if (pProp->GetCategory() == ezPropertyCategory::Member)
      {
        auto pSpecificProp = static_cast<ezAbstractMemberProperty*>(pProp);
        if (!pProp->GetFlags().AreAllSet(ezPropertyFlags::Pointer | ezPropertyFlags::PointerOwner))
        {
          ezLog::Error("Property '{0}' not a pointer, can't remove object!", change.m_Change.m_sProperty);
          return;
        }

        void* pValue = nullptr;
        pSpecificProp->SetValuePtr(object.m_pObject, &pValue);
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<ezAbstractArrayProperty*>(pProp);
        ezReflectionUtils::RemoveArrayPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.ConvertTo<ezUInt32>());
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Set)
      {
        EZ_ASSERT_DEV(pProp->GetFlags().IsSet(ezPropertyFlags::Pointer), "Set object must always be pointers!");
        auto pSpecificProp = static_cast<ezAbstractSetProperty*>(pProp);
        auto valueObject = m_pContext->GetObjectByGUID(change.m_Change.m_Value.Get<ezUuid>());
        ezReflectionUtils::RemoveSetPropertyValue(pSpecificProp, object.m_pObject, ezVariant(valueObject.m_pObject));
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<ezAbstractMapProperty*>(pProp);
        pSpecificProp->Remove(object.m_pObject, change.m_Change.m_Index.Get<ezString>());
      }

      if (pProp->GetFlags().AreAllSet(ezPropertyFlags::Pointer | ezPropertyFlags::PointerOwner))
      {
        m_pContext->DeleteObject(change.m_Change.m_Value.Get<ezUuid>());
      }
    }
    break;
    case ezObjectChangeType::PropertySet:
    {
      if (pProp->GetCategory() == ezPropertyCategory::Member)
      {
        auto pSpecificProp = static_cast<ezAbstractMemberProperty*>(pProp);
        ezReflectionUtils::SetMemberPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Value);
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<ezAbstractArrayProperty*>(pProp);
        ezReflectionUtils::SetArrayPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.ConvertTo<ezUInt32>(),
                                                 change.m_Change.m_Value);
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Set)
      {
        auto pSpecificProp = static_cast<ezAbstractSetProperty*>(pProp);
        ezReflectionUtils::InsertSetPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Value);
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<ezAbstractMapProperty*>(pProp);
        ezReflectionUtils::SetMapPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.Get<ezString>(),
                                               change.m_Change.m_Value);
      }
    }
    break;
    case ezObjectChangeType::PropertyInserted:
    {
      ezVariant value = change.m_Change.m_Value;
      if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
      {
        auto valueObject = m_pContext->GetObjectByGUID(change.m_Change.m_Value.Get<ezUuid>());
        value = valueObject.m_pObject;
      }

      if (pProp->GetCategory() == ezPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<ezAbstractArrayProperty*>(pProp);
        ezReflectionUtils::InsertArrayPropertyValue(pSpecificProp, object.m_pObject, value, change.m_Change.m_Index.ConvertTo<ezUInt32>());
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Set)
      {
        auto pSpecificProp = static_cast<ezAbstractSetProperty*>(pProp);
        ezReflectionUtils::InsertSetPropertyValue(pSpecificProp, object.m_pObject, value);
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<ezAbstractMapProperty*>(pProp);
        ezReflectionUtils::SetMapPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.Get<ezString>(), value);
      }
    }
    break;
    case ezObjectChangeType::PropertyRemoved:
    {
      if (pProp->GetCategory() == ezPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<ezAbstractArrayProperty*>(pProp);
        ezReflectionUtils::RemoveArrayPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.ConvertTo<ezUInt32>());
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Set)
      {
        ezVariant value = change.m_Change.m_Value;
        if (pProp->GetFlags().IsSet(ezPropertyFlags::Pointer))
        {
          auto valueObject = m_pContext->GetObjectByGUID(change.m_Change.m_Value.Get<ezUuid>());
          value = valueObject.m_pObject;
        }

        auto pSpecificProp = static_cast<ezAbstractSetProperty*>(pProp);
        ezReflectionUtils::RemoveSetPropertyValue(pSpecificProp, object.m_pObject, value);
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<ezAbstractMapProperty*>(pProp);
        pSpecificProp->Remove(object.m_pObject, change.m_Change.m_Index.Get<ezString>());
      }
    }
    break;
  }
}
