#include <EditorFrameworkPCH.h>

#include <Core/World/Component.h>
#include <Core/World/GameObject.h>
#include <EditorEngineProcessFramework/LongOperation/LongOperationManager.h>
#include <EditorFramework/LongOps/LongOpsAdapter.h>
#include <Foundation/Configuration/Startup.h>

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

  CheckAllTypes();
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
    const char* szDocType = e.m_pDocument->GetDocumentTypeDescriptor()->m_pDocumentType->GetTypeName();
    if (ezStringUtils::IsEqual(szDocType, "ezSceneDocument"))
    {
      e.m_pDocument->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezLongOpsAdapter::StructureEventHandler, this));

      ObjectAdded(e.m_pDocument->GetObjectManager()->GetRootObject());
    }
  }

  if (e.m_Type == ezDocumentManager::Event::Type::DocumentClosing)
  {
    const char* szDocType = e.m_pDocument->GetDocumentTypeDescriptor()->m_pDocumentType->GetTypeName();
    if (ezStringUtils::IsEqual(szDocType, "ezSceneDocument"))
    {
      ezLongOpManager::GetSingleton()->DocumentClosed(e.m_pDocument->GetGuid());

      e.m_pDocument->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
        ezMakeDelegate(&ezLongOpsAdapter::StructureEventHandler, this));
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
    // manager unregister all components
  }

  if (!bExists && e.m_Type == ezPhantomRttiManagerEvent::Type::TypeAdded)
  {
    if (e.m_pChangedType->GetAttributeByType<ezLongOpAttribute>() != nullptr)
    {
      m_TypesWithLongOps.Insert(e.m_pChangedType);
      // manager register all components
    }
  }

  if (e.m_Type == ezPhantomRttiManagerEvent::Type::TypeChanged)
  {
    // TODO
  }
}

void ezLongOpsAdapter::CheckAllTypes()
{
  for (const ezRTTI* pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetParentType())
  {
    if (pRtti->GetAttributeByType<ezLongOpAttribute>() != nullptr)
    {
      m_TypesWithLongOps.Insert(pRtti);
    }
  }
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
        for (ezPropertyAttribute* pAttr : pRtti->GetAttributes())
        {
          if (auto pOpAttr = ezDynamicCast<ezLongOpAttribute*>(pAttr))
          {
            ezLongOpManager::GetSingleton()->RegisterLongOp(
              pObject->GetDocumentObjectManager()->GetDocument()->GetGuid(), pObject->GetGuid(), pOpAttr->m_sOpTypeName);
          }
        }

        pRtti = pRtti->GetParentType();
      }
    }
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
        for (ezPropertyAttribute* pAttr : pRtti->GetAttributes())
        {
          if (auto pOpAttr = ezDynamicCast<ezLongOpAttribute*>(pAttr))
          {
            ezLongOpManager::GetSingleton()->UnregisterLongOp(
              pObject->GetDocumentObjectManager()->GetDocument()->GetGuid(), pObject->GetGuid(), pOpAttr->m_sOpTypeName);
          }
        }

        pRtti = pRtti->GetParentType();
      }
    }
  }

  if (pRtti->IsDerivedFrom<ezGameObject>())
  {
    for (const ezDocumentObject* pChild : pObject->GetChildren())
    {
      ObjectRemoved(pChild);
    }
  }
}
