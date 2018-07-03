#include <PCH.h>

#include <EditorFramework/Manipulators/ManipulatorAdapterRegistry.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>

EZ_IMPLEMENT_SINGLETON(ezManipulatorAdapterRegistry);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, ManipulatorAdapterRegistry)
 
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ManipulatorManager"
  END_SUBSYSTEM_DEPENDENCIES
 
  ON_CORE_STARTUP
  {
    EZ_DEFAULT_NEW(ezManipulatorAdapterRegistry);
  }
 
  ON_CORE_SHUTDOWN
  {
    auto ptr = ezManipulatorAdapterRegistry::GetSingleton();
    EZ_DEFAULT_DELETE(ptr);
  }
 
EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezManipulatorAdapterRegistry::ezManipulatorAdapterRegistry()
    : m_SingletonRegistrar(this)
{
  ezManipulatorManager::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezManipulatorAdapterRegistry::ManipulatorManagerEventHandler, this));
}

ezManipulatorAdapterRegistry::~ezManipulatorAdapterRegistry()
{
  ezManipulatorManager::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezManipulatorAdapterRegistry::ManipulatorManagerEventHandler, this));

  for (auto it = m_DocumentAdapters.GetIterator(); it.IsValid(); ++it)
  {
    ClearAdapters(it.Key());
  }
}

void ezManipulatorAdapterRegistry::ManipulatorManagerEventHandler(const ezManipulatorManagerEvent& e)
{
  ClearAdapters(e.m_pDocument);

  if (e.m_pManipulator == nullptr || e.m_bHideManipulators)
    return;

  for (const auto& sel : *e.m_pSelection)
  {
    ezManipulatorAdapter* pAdapter = m_Factory.CreateObject(e.m_pManipulator->GetDynamicRTTI());

    if (pAdapter)
    {
      m_DocumentAdapters[e.m_pDocument].m_Adapters.PushBack(pAdapter);
      pAdapter->SetManipulator(e.m_pManipulator, sel.m_pObject);
    }
  }
}

void ezManipulatorAdapterRegistry::ClearAdapters(const ezDocument* pDocument)
{
  for (auto& adapt : m_DocumentAdapters[pDocument].m_Adapters)
  {
    EZ_DEFAULT_DELETE(adapt);
  }

  m_DocumentAdapters[pDocument].m_Adapters.Clear();
}
