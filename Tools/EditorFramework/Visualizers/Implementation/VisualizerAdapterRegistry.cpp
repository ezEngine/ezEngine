#include <PCH.h>

#include <EditorFramework/Visualizers/VisualizerAdapterRegistry.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>

EZ_IMPLEMENT_SINGLETON(ezVisualizerAdapterRegistry);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, VisualizerAdapterRegistry)
 
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "VisualizerManager"
  END_SUBSYSTEM_DEPENDENCIES
 
  ON_CORE_STARTUP
  {
    EZ_DEFAULT_NEW(ezVisualizerAdapterRegistry);
  }
 
  ON_CORE_SHUTDOWN
  {
    auto ptr = ezVisualizerAdapterRegistry::GetSingleton();
    EZ_DEFAULT_DELETE(ptr);
  }
 
EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezVisualizerAdapterRegistry::ezVisualizerAdapterRegistry()
    : m_SingletonRegistrar(this)
{
  ezVisualizerManager::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezVisualizerAdapterRegistry::VisualizerManagerEventHandler, this));
}

ezVisualizerAdapterRegistry::~ezVisualizerAdapterRegistry()
{
  ezVisualizerManager::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezVisualizerAdapterRegistry::VisualizerManagerEventHandler, this));

  for (auto it = m_DocumentAdapters.GetIterator(); it.IsValid(); ++it)
  {
    ClearAdapters(it.Key());
  }
}


void ezVisualizerAdapterRegistry::CreateAdapters(const ezDocument* pDocument, const ezDocumentObject* pObject)
{
  const auto& attributes = pObject->GetTypeAccessor().GetType()->GetAttributes();

  for (const auto pAttr : attributes)
  {
    if (pAttr->IsInstanceOf<ezVisualizerAttribute>())
    {
      ezVisualizerAdapter* pAdapter = m_Factory.CreateObject(pAttr->GetDynamicRTTI());

      if (pAdapter)
      {
        m_DocumentAdapters[pDocument].m_Adapters.PushBack(pAdapter);
        pAdapter->SetVisualizer(static_cast<ezVisualizerAttribute*>(pAttr), pObject);
      }
    }
  }

  for (const auto pChild : pObject->GetChildren())
  {
    CreateAdapters(pDocument, pChild);
  }
}

void ezVisualizerAdapterRegistry::VisualizerManagerEventHandler(const ezVisualizerManagerEvent& e)
{
  ClearAdapters(e.m_pDocument);

  for (const auto sel : *e.m_pSelection)
  {
    CreateAdapters(e.m_pDocument, sel);
  }
}

void ezVisualizerAdapterRegistry::ClearAdapters(const ezDocument* pDocument)
{
  for (auto& adapt : m_DocumentAdapters[pDocument].m_Adapters)
  {
    EZ_DEFAULT_DELETE(adapt);
  }

  m_DocumentAdapters[pDocument].m_Adapters.Clear();
}
