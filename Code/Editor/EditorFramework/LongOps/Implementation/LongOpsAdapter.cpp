#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorFramework/LongOps/LongOpsAdapter.h>

EZ_IMPLEMENT_SINGLETON(ezLongOpsAdapter);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, LongOpsAdapter)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager",
    "DocumentManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    EZ_DEFAULT_NEW(ezLongOpsAdapter);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (ezLongOpsAdapter::GetSingleton())
    {
      auto ptr = ezLongOpsAdapter::GetSingleton();
      EZ_DEFAULT_DELETE(ptr);
    }
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezLongOpsAdapter::ezLongOpsAdapter()
  : m_SingletonRegistrar(this)
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezLongOpsAdapter::DocumentManagerEventHandler, this));
  ezPhantomRttiManager::s_Events.AddEventHandler(ezMakeDelegate(&ezLongOpsAdapter::PhantomTypeRegistryEventHandler, this));
}

ezLongOpsAdapter::~ezLongOpsAdapter()
{
  ezPhantomRttiManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezLongOpsAdapter::PhantomTypeRegistryEventHandler, this));
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezLongOpsAdapter::DocumentManagerEventHandler, this));
}

void ezLongOpsAdapter::DocumentManagerEventHandler(const ezDocumentManager::Event& e)
{
  if (e.m_Type == ezDocumentManager::Event::Type::DocumentOpened)
  {
    const ezRTTI* pRttiScene = ezRTTI::FindTypeByName("ezSceneDocument");
    const bool bIsScene = e.m_pDocument->GetDocumentTypeDescriptor()->m_pDocumentType->IsDerivedFrom(pRttiScene);
    if (bIsScene)
    {
      CheckAllTypes();

      e.m_pDocument->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezLongOpsAdapter::StructureEventHandler, this));

      ObjectAdded(e.m_pDocument->GetObjectManager()->GetRootObject());
    }
  }

  if (e.m_Type == ezDocumentManager::Event::Type::DocumentClosing)
  {
    const ezRTTI* pRttiScene = ezRTTI::FindTypeByName("ezSceneDocument");
    const bool bIsScene = e.m_pDocument->GetDocumentTypeDescriptor()->m_pDocumentType->IsDerivedFrom(pRttiScene);
    if (bIsScene)
    {
      ezLongOpControllerManager::GetSingleton()->CancelAndRemoveAllOpsForDocument(e.m_pDocument->GetGuid());

      e.m_pDocument->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezLongOpsAdapter::StructureEventHandler, this));
    }
  }
}

void ezLongOpsAdapter::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  if (e.m_EventType == ezDocumentObjectStructureEvent::Type::AfterObjectAdded)
  {
    ObjectAdded(e.m_pObject);
  }

  if (e.m_EventType == ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved)
  {
    ObjectRemoved(e.m_pObject);
  }
}

void ezLongOpsAdapter::PhantomTypeRegistryEventHandler(const ezPhantomRttiManagerEvent& e)
{
  const bool bExists = m_TypesWithLongOps.Contains(e.m_pChangedType);

  if (bExists && e.m_Type == ezPhantomRttiManagerEvent::Type::TypeRemoved)
  {
    m_TypesWithLongOps.Remove(e.m_pChangedType);
    // if this ever becomes relevant:
    // iterate over all open documents and figure out which long ops to remove
  }

  if (!bExists && e.m_Type == ezPhantomRttiManagerEvent::Type::TypeAdded)
  {
    if (e.m_pChangedType->GetAttributeByType<ezLongOpAttribute>() != nullptr)
    {
      m_TypesWithLongOps.Insert(e.m_pChangedType);
      // if this ever becomes relevant:
      // iterate over all open documents and figure out which long ops to add
    }
  }

  if (e.m_Type == ezPhantomRttiManagerEvent::Type::TypeChanged)
  {
    // if this ever becomes relevant:
    // iterate over all open documents and figure out which long ops to add or remove
  }
}

void ezLongOpsAdapter::CheckAllTypes()
{
  ezRTTI::ForEachType(
    [&](const ezRTTI* pRtti)
    {
      if (pRtti->GetAttributeByType<ezLongOpAttribute>() != nullptr)
      {
        m_TypesWithLongOps.Insert(pRtti);
      }
    });
}

void ezLongOpsAdapter::ObjectAdded(const ezDocumentObject* pObject)
{
  const ezRTTI* pRtti = pObject->GetType();

  if (pRtti->IsDerivedFrom<ezComponent>())
  {
    if (m_TypesWithLongOps.Contains(pRtti))
    {
      while (pRtti)
      {
        for (const ezPropertyAttribute* pAttr : pRtti->GetAttributes())
        {
          if (auto pOpAttr = ezDynamicCast<const ezLongOpAttribute*>(pAttr))
          {
            ezLongOpControllerManager::GetSingleton()->RegisterLongOp(pObject->GetDocumentObjectManager()->GetDocument()->GetGuid(), pObject->GetGuid(), pOpAttr->m_sOpTypeName);
          }
        }

        pRtti = pRtti->GetParentType();
      }
    }

    return;
  }

  if (pRtti->IsDerivedFrom<ezGameObject>() || pObject->GetParent() == nullptr /*document root object*/)
  {
    for (const ezDocumentObject* pChild : pObject->GetChildren())
    {
      ObjectAdded(pChild);
    }
  }
}

void ezLongOpsAdapter::ObjectRemoved(const ezDocumentObject* pObject)
{
  const ezRTTI* pRtti = pObject->GetType();

  if (pRtti->IsDerivedFrom<ezComponent>())
  {
    if (m_TypesWithLongOps.Contains(pRtti))
    {
      while (pRtti)
      {
        for (const ezPropertyAttribute* pAttr : pRtti->GetAttributes())
        {
          if (auto pOpAttr = ezDynamicCast<const ezLongOpAttribute*>(pAttr))
          {
            ezLongOpControllerManager::GetSingleton()->UnregisterLongOp(pObject->GetDocumentObjectManager()->GetDocument()->GetGuid(), pObject->GetGuid(), pOpAttr->m_sOpTypeName);
          }
        }

        pRtti = pRtti->GetParentType();
      }
    }
  }
  else if (pRtti->IsDerivedFrom<ezGameObject>())
  {
    for (const ezDocumentObject* pChild : pObject->GetChildren())
    {
      ObjectRemoved(pChild);
    }
  }
}
