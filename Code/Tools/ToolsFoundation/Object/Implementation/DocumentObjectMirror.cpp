#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Serialization/JsonSerializer.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezObjectChangeType, 1)
EZ_ENUM_CONSTANTS(ezObjectChangeType::ObjectAdded, ezObjectChangeType::ObjectRemoved)
EZ_ENUM_CONSTANTS(ezObjectChangeType::PropertySet, ezObjectChangeType::PropertyInserted, ezObjectChangeType::PropertyRemoved)
EZ_END_STATIC_REFLECTED_ENUM();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezObjectChangeStep, ezNoBase, 1, ezRTTIDefaultAllocator<ezObjectChangeStep>);
EZ_BEGIN_PROPERTIES
EZ_MEMBER_PROPERTY("Property", m_sProperty),
EZ_MEMBER_PROPERTY("Index", m_Index),
EZ_MEMBER_PROPERTY("Value", m_Value),
EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezObjectChange, ezNoBase, 1, ezRTTIDefaultAllocator<ezObjectChange>);
EZ_BEGIN_PROPERTIES
EZ_ENUM_MEMBER_PROPERTY("Type", ezObjectChangeType, m_Type),
EZ_MEMBER_PROPERTY("Root", m_Root),
EZ_ARRAY_MEMBER_PROPERTY("Steps", m_Steps),
EZ_MEMBER_PROPERTY("Change", m_Change),
EZ_MEMBER_PROPERTY("Graph", m_sGraph),
EZ_END_PROPERTIES
EZ_END_STATIC_REFLECTED_TYPE();

ezObjectChange::ezObjectChange(const ezObjectChange&)
{
  EZ_REPORT_FAILURE("Not supported!");
}

void ezObjectChange::GetGraph(ezAbstractObjectGraph& graph) const
{
  graph.Clear();
  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  writer.WriteBytes(m_sGraph.GetData(), m_sGraph.GetElementCount());
  ezMemoryStreamReader reader(&storage);
  ezAbstractGraphJsonSerializer::Read(reader, &graph);
}

void ezObjectChange::SetGraph(ezAbstractObjectGraph& graph)
{
  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezAbstractGraphJsonSerializer::Write(writer, &graph, ezJSONWriter::WhitespaceMode::LessIndentation);

  m_sGraph = ezStringView((const char*)storage.GetData(), (const char*)storage.GetData() + storage.GetStorageSize());
}

ezObjectChange::ezObjectChange( ezObjectChange && rhs )
{
	m_Type = rhs.m_Type;
	m_Root = rhs.m_Root;
	m_Steps = std::move( rhs.m_Steps );
	m_Change = std::move( rhs.m_Change );
	m_sGraph = std::move( rhs.m_sGraph );
}

void ezObjectChange::operator=(ezObjectChange&& rhs)
{
  m_Type = rhs.m_Type;
  m_Root = rhs.m_Root;
  m_Steps = std::move(rhs.m_Steps);
  m_Change = std::move(rhs.m_Change);
  m_sGraph = std::move(rhs.m_sGraph);
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

void ezDocumentObjectMirror::SendDocument()
{
  const auto* pRoot = m_pManager->GetRootObject();
  for (auto* pChild : pRoot->GetChildren())
  {
    ezObjectChange change;
    change.m_Type = ezObjectChangeType::ObjectAdded;
    change.m_Change.m_Value = pChild->GetGuid();

    ezAbstractObjectGraph graph;
    ezDocumentObjectConverterWriter objectConverter(&graph, m_pManager, true, true);
    ezAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(pChild, "Object");
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
      change.m_Type = ezObjectChangeType::ObjectRemoved;
      change.m_Change.m_Value = pChild->GetGuid();

      /*ezAbstractObjectGraph graph;
      ezDocumentObjectConverterWriter objectConverter(&graph, m_pManager, true, true);
      ezAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(pChild, "Object");
      change.SetGraph(graph);*/

      ApplyOp(change);
    }
  }
  
}

void ezDocumentObjectMirror::TreeStructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved:
    {
      if (IsHeapAllocated(e.m_pNewParent, e.m_sParentProperty))
      {
        if (e.m_pNewParent == nullptr || e.m_pNewParent == m_pManager->GetRootObject())
        {
          // Object is now a root object, nothing to do to attach it to its new parentparent.
          break;
        }
        ezObjectChange change;
        ezPropertyPath propPath = e.m_sParentProperty.GetData();
        CreatePath(change, e.m_pNewParent, propPath);

        change.m_Type = ezObjectChangeType::PropertyInserted;
        change.m_Change.m_Index = e.m_PropertyIndex;
        change.m_Change.m_Value = e.m_pObject->GetGuid();

        ApplyOp(change);
        break;
      }
      // Intended falltrough as non ptr object might as well be destroyed and rebuild.
    }
    //case ezDocumentObjectStructureEvent::Type::BeforeObjectAdded:
  case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      ezObjectChange change;
      ezPropertyPath propPath = e.m_sParentProperty.GetData();
      CreatePath(change, e.m_pNewParent, propPath);

      change.m_Type = ezObjectChangeType::ObjectAdded;
      change.m_Change.m_Index = e.m_PropertyIndex;
      change.m_Change.m_Value = e.m_pObject->GetGuid();

      ezAbstractObjectGraph graph;
      ezDocumentObjectConverterWriter objectConverter(&graph, m_pManager, true, true);
      ezAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(e.m_pObject, "Object");
      change.SetGraph(graph);

      ApplyOp(change);
    }
    break;
  case ezDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    {
      if (IsHeapAllocated(e.m_pPreviousParent, e.m_sParentProperty))
      {
        EZ_ASSERT_DEBUG(IsHeapAllocated(e.m_pNewParent, e.m_sParentProperty), "Old and new parent must have the same heap allocation state!");
        if (e.m_pPreviousParent == nullptr || e.m_pPreviousParent == m_pManager->GetRootObject())
        {
          // Object is currently a root object, nothing to do to detach it from its parent.
          break;
        }

        ezObjectChange change;
        ezPropertyPath propPath = e.m_sParentProperty.GetData();
        CreatePath(change, e.m_pPreviousParent, propPath);

        change.m_Type = ezObjectChangeType::PropertyRemoved;
        change.m_Change.m_Index = e.m_pObject->GetPropertyIndex();
        change.m_Change.m_Value = e.m_pObject->GetGuid();

        ApplyOp(change);
        break;
      }
      // Intended falltrough as non ptr object might as well be destroyed and rebuild.
    }
    //case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
  case ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      ezObjectChange change;
      ezPropertyPath propPath = e.m_sParentProperty.GetData();
      CreatePath(change, e.m_pPreviousParent, propPath);

      change.m_Type = ezObjectChangeType::ObjectRemoved;
      change.m_Change.m_Index = e.m_PropertyIndex;
      change.m_Change.m_Value = e.m_pObject->GetGuid();

      ApplyOp(change);
    }
    break;
  }
}

void ezDocumentObjectMirror::TreePropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  const ezPropertyPath propPath = e.m_sPropertyPath.GetData();

  switch (e.m_EventType)
  {
  case ezDocumentObjectPropertyEvent::Type::PropertySet:
    {
      ezObjectChange change;
      CreatePath(change, e.m_pObject, propPath);

      change.m_Type = ezObjectChangeType::PropertySet;
      change.m_Change.m_Index = e.m_NewIndex;
      change.m_Change.m_Value = e.m_NewValue;
      ApplyOp(change);
    }
    break;
  case ezDocumentObjectPropertyEvent::Type::PropertyInserted:
    {
      ezObjectChange change;
      CreatePath(change, e.m_pObject, propPath);

      change.m_Type = ezObjectChangeType::PropertyInserted;
      change.m_Change.m_Index = e.m_NewIndex;
      change.m_Change.m_Value = e.m_NewValue;
      ApplyOp(change);
    }
    break;
  case ezDocumentObjectPropertyEvent::Type::PropertyRemoved:
    {
      ezObjectChange change;
      CreatePath(change, e.m_pObject, propPath);

      change.m_Type = ezObjectChangeType::PropertyRemoved;
      change.m_Change.m_Index = e.m_OldIndex;
      change.m_Change.m_Value = e.m_OldValue;
      ApplyOp(change);
    }
    break;
  case ezDocumentObjectPropertyEvent::Type::PropertyMoved:
    {
      ezUInt32 uiOldIndex = e.m_OldIndex.ConvertTo<ezUInt32>();
      ezUInt32 uiNewIndex = e.m_NewIndex.ConvertTo<ezUInt32>();

      {
        ezObjectChange change;
        CreatePath(change, e.m_pObject, propPath);

        change.m_Type = ezObjectChangeType::PropertyRemoved;
        change.m_Change.m_Index = uiOldIndex;
        change.m_Change.m_Value = e.m_NewValue;
        EZ_ASSERT_DEBUG(e.m_NewValue.IsValid(), "Value must be valid");
        ApplyOp(change);
      }

      if (uiNewIndex > uiOldIndex)
      {
        uiNewIndex -= 1;
      }

      {
        ezObjectChange change;
        CreatePath(change, e.m_pObject, propPath);

        change.m_Type = ezObjectChangeType::PropertyInserted;
        change.m_Change.m_Index = uiNewIndex;
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

  ezPropertyPath path(szParentProperty);
  auto* pProp = ezToolsReflectionUtils::GetPropertyByPath(pRtti, path);
  return pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner);
}

void ezDocumentObjectMirror::CreatePath(ezObjectChange& out_change, const ezDocumentObject* pRoot, const ezPropertyPath& propPath)
{
  if (pRoot && pRoot->GetDocumentObjectManager()->GetRootObject() != pRoot)
  {
    ezHybridArray<const ezDocumentObject*, 8> path;
    out_change.m_Root = FindRootOpObject(pRoot, path);
    FlattenSteps(path, out_change.m_Steps);
  }

  if (!propPath.IsEmpty())
  {
    for (ezUInt32 j = 0; j < propPath.GetCount() - 1; ++j)
    {
      out_change.m_Steps.PushBack(ezObjectChangeStep(propPath[j], ezVariant()));
    }
    out_change.m_Change.m_sProperty = propPath.PeekBack();
  }
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

void ezDocumentObjectMirror::FlattenSteps(const ezArrayPtr<const ezDocumentObject* const> path, ezHybridArray<ezObjectChangeStep, 4>& out_steps)
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
    AddPathToSteps(pObject->GetParentProperty(), pObject->GetPropertyIndex(), out_steps);
  }
}

void ezDocumentObjectMirror::AddPathToSteps(const char* szPropertyPath, const ezVariant& index, ezHybridArray<ezObjectChangeStep, 4>& out_steps, bool bAddLastProperty)
{
  ezPropertyPath propPath = szPropertyPath;
  for (ezUInt32 j = 0; j < propPath.GetCount() - 1; ++j)
  {
    out_steps.PushBack(ezObjectChangeStep(propPath[j], ezVariant()));
  }

  if (bAddLastProperty)
    out_steps.PushBack(ezObjectChangeStep(propPath.PeekBack(), index));
}

void ezDocumentObjectMirror::ApplyOp(ezObjectChange& change)
{
  ezRttiConverterObject object;
  if (change.m_Root.IsValid())
  {
    object = m_pContext->GetObjectByGUID(change.m_Root);
    EZ_ASSERT_DEV(object.m_pObject != nullptr, "Root objext does not exist in mirrored native object!");
  }
  RetrieveObject(object, change, change.m_Steps);
}

void ezDocumentObjectMirror::ApplyOp(ezRttiConverterObject object, const ezObjectChange& change)
{
  ezAbstractProperty* pProp = nullptr;
  
  if (object.m_pType != nullptr)
  {
    pProp = object.m_pType->FindPropertyByName(change.m_Change.m_sProperty);
    if (pProp == nullptr)
    {
      ezLog::Error("Property '%s' not found, can't apply mirror op!", change.m_Change.m_sProperty.GetData());
      return;
    }
  }

  switch (change.m_Type)
  {
  case ezObjectChangeType::ObjectAdded:
    {
      ezAbstractObjectGraph graph;
      change.GetGraph(graph);
      ezRttiConverterReader reader(&graph, m_pContext);
      const ezAbstractObjectNode* pNode = graph.GetNodeByName("Object");

      void* pValue = reader.CreateObjectFromNode(pNode);
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
        ezReflectionUtils::InsertSetPropertyValue(pSpecificProp, object.m_pObject, pValue);
      }

      if (!pProp->GetFlags().AreAllSet(ezPropertyFlags::Pointer | ezPropertyFlags::PointerOwner))
      {
        m_pContext->DeleteObject(pNode->GetGuid());
      }
    }
    break;
  case ezObjectChangeType::ObjectRemoved:
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
          ezLog::Error("Property '%s' not a pointer, can't remove object!", change.m_Change.m_sProperty.GetData());
          return;
        }

        void* pValue = nullptr;
        pSpecificProp->SetValuePtr(object.m_pObject, &pValue);
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<ezAbstractArrayProperty*>(pProp);
        pSpecificProp->Remove(object.m_pObject, change.m_Change.m_Index.ConvertTo<ezUInt32>());
      }
      else if (pProp->GetCategory() == ezPropertyCategory::Set)
      {
        EZ_ASSERT_DEV(pProp->GetFlags().IsSet(ezPropertyFlags::Pointer), "Set object must always be pointers!");
        auto pSpecificProp = static_cast<ezAbstractSetProperty*>(pProp);
        auto valueObject = m_pContext->GetObjectByGUID(change.m_Change.m_Value.Get<ezUuid>());
        ezReflectionUtils::RemoveSetPropertyValue(pSpecificProp, object.m_pObject, valueObject.m_pObject);
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
        ezReflectionUtils::SetArrayPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.ConvertTo<ezUInt32>(), change.m_Change.m_Value);
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
    }
    break;
  }
}

void ezDocumentObjectMirror::RetrieveObject(ezRttiConverterObject object, const ezObjectChange& change, const ezArrayPtr<const ezObjectChangeStep> path)
{
  const ezUInt32 uiCount = path.GetCount();

  // Destination reached? (end recursion)
  if (uiCount == 0)
  {
    ApplyOp(object, change);
    return;
  }
  else // Recurse
  {
    const ezRTTI* pCurrentType = object.m_pType;
    auto pProp = pCurrentType->FindPropertyByName(path[0].m_sProperty);
    const ezRTTI* pPropType = pProp->GetSpecificType();
    switch (pProp->GetCategory())
    {
    case ezPropertyCategory::Member:
      {
        ezAbstractMemberProperty* pSpecific = static_cast<ezAbstractMemberProperty*>(pProp);
        EZ_ASSERT_DEV(!pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags), "");
        EZ_ASSERT_DEV(!pProp->GetFlags().IsSet(ezPropertyFlags::StandardType), "");
        EZ_ASSERT_DEV(!pProp->GetFlags().IsSet(ezPropertyFlags::Pointer), "");

        if (pPropType->GetProperties().GetCount() > 0)
        {
          ezRttiConverterObject subObject;
          subObject.m_pObject = pSpecific->GetPropertyPointer(object.m_pObject);
          subObject.m_pType = pPropType;
          // Do we have direct access to the property?
          if (subObject.m_pObject != nullptr)
          {
            RetrieveObject(subObject, change, path.GetSubArray(1));
          }
          // If the property is behind an accessor, we need to retrieve it first.
          else if (pPropType->GetAllocator()->CanAllocate())
          {
            subObject.m_pObject = pPropType->GetAllocator()->Allocate();
            pSpecific->GetValuePtr(object.m_pObject, subObject.m_pObject);

            RetrieveObject(subObject, change, path.GetSubArray(1));

            pSpecific->SetValuePtr(object.m_pObject, subObject.m_pObject);
            pPropType->GetAllocator()->Deallocate(subObject.m_pObject);
          }
          else
          {
            EZ_REPORT_FAILURE("Non-allocatable property should not be part of an object chain!");
          }
        }
      }
      break;
    case ezPropertyCategory::Array:
      {
        ezAbstractArrayProperty* pSpecific = static_cast<ezAbstractArrayProperty*>(pProp);
        ezUInt32 uiArrayCount = pSpecific->GetCount(object.m_pObject);

        EZ_ASSERT_DEV(!pProp->GetFlags().IsSet(ezPropertyFlags::StandardType), "");
        EZ_ASSERT_DEV(!pProp->GetFlags().IsSet(ezPropertyFlags::Pointer), "");

        if (pPropType->GetAllocator()->CanAllocate())
        {
          ezRttiConverterObject subObject;
          subObject.m_pObject = pPropType->GetAllocator()->Allocate();
          subObject.m_pType = pPropType;
          pSpecific->GetValue(object.m_pObject, path[0].m_Index.ConvertTo<ezUInt32>(), subObject.m_pObject);

          RetrieveObject(subObject, change, path.GetSubArray(1));

          pSpecific->SetValue(object.m_pObject, path[0].m_Index.ConvertTo<ezUInt32>(), subObject.m_pObject);
          pPropType->GetAllocator()->Deallocate(subObject.m_pObject);
        }
        else
        {
          EZ_REPORT_FAILURE("Non-allocatable property should not be part of an object chain!");
        }
      }
      break;
    case ezPropertyCategory::Set:
    default:
      {
        EZ_REPORT_FAILURE("Property of type Set should not be part of an object chain!");
      }
      break;
    }
  }

}
