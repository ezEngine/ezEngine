#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

ezDocumentObjectMirror::ezDocumentObjectMirror()
{
  m_pManager = nullptr;
}

ezDocumentObjectMirror::~ezDocumentObjectMirror()
{
}

void ezDocumentObjectMirror::Init(const ezDocumentObjectManager* pManager)
{
  m_pManager = pManager;
  m_pManager->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezDocumentObjectMirror::TreeStructureEventHandler, this));
  m_pManager->m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezDocumentObjectMirror::TreePropertyEventHandler, this));

  const auto* pRoot = m_pManager->GetRootObject();
  for (auto* pChild : pRoot->GetChildren())
  {
    ezAbstractObjectGraph graph;
    ezDocumentObjectConverterWriter objectConverter(&graph, m_pManager, true, true);
    ezAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(pChild, "Object");
    ezRttiConverterReader reader(&graph, &m_Context);
    reader.CreateObjectFromNode(pNode);
  }
}

void ezDocumentObjectMirror::DeInit()
{
  m_pManager->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezDocumentObjectMirror::TreeStructureEventHandler, this));
  m_pManager->m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezDocumentObjectMirror::TreePropertyEventHandler, this));

  const auto* pRoot = m_pManager->GetRootObject();
  for (auto* pChild : pRoot->GetChildren())
  {
    m_Context.DeleteObject(pChild->GetGuid());
  }
}

void ezDocumentObjectMirror::TreeStructureEventHandler(const ezDocumentObjectStructureEvent& e)
{


  switch (e.m_EventType)
  {
    //case ezDocumentObjectStructureEvent::Type::BeforeObjectAdded:
  case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
  case ezDocumentObjectStructureEvent::Type::AfterObjectMoved:
    {
      if (IsRootObject(e.m_pNewParent))
      {
        // Create without parent
        ezAbstractObjectGraph graph;
        ezDocumentObjectConverterWriter objectConverter(&graph, m_pManager, true, true);
        ezAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(e.m_pObject, "Object");
        ezRttiConverterReader reader(&graph, &m_Context);
        reader.CreateObjectFromNode(pNode);
      }
      else
      {
        ezHybridArray<const ezDocumentObjectBase*, 8> path;
        ezUuid guid = FindRootOpObject(e.m_pNewParent, path);

        // Recreate parent guid in place with all children
        ezAbstractObjectGraph graph;
        ezDocumentObjectConverterWriter objectConverter(&graph, m_pManager, true, true);
        ezAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(path[path.GetCount() - 1], "Object");
        ezRttiConverterReader reader(&graph, &m_Context);
        auto* pObject = m_Context.GetObjectByGUID(guid);
        EZ_ASSERT_DEV(pObject != nullptr, "Object was not created but is in the tree!");
        reader.ApplyPropertiesToObject(pNode, pObject->m_pType, pObject->m_pObject);
      }
    }
    break;
  case ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    //case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
  case ezDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    {
      if (IsRootObject(e.m_pPreviousParent))
      {
        // Just delete
        m_Context.DeleteObject(e.m_pObject->GetGuid());
      }
      else
      {
        ezHybridArray<const ezDocumentObjectBase*, 8> path;
        ezUuid guid = FindRootOpObject(e.m_pPreviousParent, path);

        // Recreate parent guid in place with all children
        ezAbstractObjectGraph graph;
        ezDocumentObjectConverterWriter objectConverter(&graph, m_pManager, true, true);
        ezAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(path[path.GetCount() - 1], "Object");
        ezRttiConverterReader reader(&graph, &m_Context);
        auto* pObject = m_Context.GetObjectByGUID(guid);
        EZ_ASSERT_DEV(pObject != nullptr, "Object was not created but is in the tree!");
        reader.ApplyPropertiesToObject(pNode, pObject->m_pType, pObject->m_pObject);
      }
    }
    break;
  }
}

void ezDocumentObjectMirror::TreePropertyEventHandler(const ezDocumentObjectPropertyEvent& e)
{
  switch (e.m_EventType)
  {
  case ezDocumentObjectPropertyEvent::Type::PropertySet:
  case ezDocumentObjectPropertyEvent::Type::PropertyInserted:
  case ezDocumentObjectPropertyEvent::Type::PropertyRemoved:
  case ezDocumentObjectPropertyEvent::Type::PropertyMoved:
    {
      if (IsRootObject(e.m_pObject->GetParent()))
      {
        //Push directly
        ezAbstractObjectGraph graph;
        ezDocumentObjectConverterWriter objectConverter(&graph, m_pManager, true, true);
        ezAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(e.m_pObject, "Object");
        ezRttiConverterReader reader(&graph, &m_Context);
        auto* pObject = m_Context.GetObjectByGUID(e.m_pObject->GetGuid());
        EZ_ASSERT_DEV(pObject != nullptr, "Object was not created but is in the tree!");
        reader.ApplyPropertiesToObject(pNode, pObject->m_pType, pObject->m_pObject);
      }
      else
      {
        ezHybridArray<const ezDocumentObjectBase*, 8> path;
        ezUuid guid = FindRootOpObject(e.m_pObject, path);

        // Recreate parent guid in place with all children
        ezAbstractObjectGraph graph;
        ezDocumentObjectConverterWriter objectConverter(&graph, m_pManager, true, true);
        ezAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(path[path.GetCount() - 1], "Object");
        ezRttiConverterReader reader(&graph, &m_Context);
        auto* pObject = m_Context.GetObjectByGUID(guid);
        EZ_ASSERT_DEV(pObject != nullptr, "Object was not created but is in the tree!");
        reader.ApplyPropertiesToObject(pNode, pObject->m_pType, pObject->m_pObject);
      }
    }
    break;
  }
}


void* ezDocumentObjectMirror::GetNativeObjectPointer(const ezDocumentObjectBase* pObject)
{
  auto* pWrapper = m_Context.GetObjectByGUID(pObject->GetGuid());

  if (pWrapper == nullptr)
    return nullptr;

  return pWrapper->m_pObject;
}

const void* ezDocumentObjectMirror::GetNativeObjectPointer(const ezDocumentObjectBase* pObject) const
{
  auto* pWrapper = m_Context.GetObjectByGUID(pObject->GetGuid());

  if (pWrapper == nullptr)
    return nullptr;

  return pWrapper->m_pObject;
}

bool ezDocumentObjectMirror::IsRootObject(const ezDocumentObjectBase* pParent)
{
  return (pParent == nullptr || pParent == m_pManager->GetRootObject());
}

bool ezDocumentObjectMirror::IsHeapAllocated(const ezDocumentObjectBase* pParent, const char* szParentProperty)
{
  if (pParent == nullptr || pParent == m_pManager->GetRootObject())
    return true;

  const ezRTTI* pRtti = pParent->GetTypeAccessor().GetType();

  ezPropertyPath path(szParentProperty);
  auto* pProp = ezToolsReflectionUtils::GetPropertyByPath(pRtti, path);
  return pProp->GetFlags().IsSet(ezPropertyFlags::PointerOwner);
}

ezUuid ezDocumentObjectMirror::FindRootOpObject(const ezDocumentObjectBase* pParent, ezHybridArray<const ezDocumentObjectBase*, 8>& path)
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

