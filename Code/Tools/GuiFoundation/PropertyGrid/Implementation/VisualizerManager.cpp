#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

EZ_IMPLEMENT_SINGLETON(ezVisualizerManager);

EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, VisualizerManager)

  ON_CORE_STARTUP
  {
    EZ_DEFAULT_NEW(ezVisualizerManager);
  }

  ON_CORE_SHUTDOWN
  {
    if (ezVisualizerManager::GetSingleton())
    {
      auto ptr = ezVisualizerManager::GetSingleton();
      EZ_DEFAULT_DELETE(ptr);
    }
  }

EZ_END_SUBSYSTEM_DECLARATION

ezVisualizerManager::ezVisualizerManager()
  : m_SingletonRegistrar(this)
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezVisualizerManager::DocumentManagerEventHandler, this));
}

ezVisualizerManager::~ezVisualizerManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezVisualizerManager::DocumentManagerEventHandler, this));
}

void ezVisualizerManager::SetVisualizersActive(const ezDocument* pDoc, bool bActive)
{
  if (m_DocsSubscribed[pDoc].m_bActivated == bActive)
    return;

  m_DocsSubscribed[pDoc].m_bActivated = bActive;

  if (!bActive)
  {
    ClearActiveVisualizers(pDoc);
  }
  else
  {
    SendEvent(pDoc);
  }
}

void ezVisualizerManager::ClearActiveVisualizers(const ezDocument* pDoc)
{
  ezDeque<const ezDocumentObject*> sel;

  ezVisualizerManagerEvent e;
  e.m_pSelection = &sel;
  e.m_pDocument = pDoc;

  m_Events.Broadcast(e);
}

void ezVisualizerManager::SelectionEventHandler(const ezSelectionManagerEvent& event)
{
  if (!m_DocsSubscribed[event.m_pDocument].m_bActivated)
    return;

  SendEvent(event.m_pDocument);
}

void ezVisualizerManager::SendEvent(const ezDocument* pDoc)
{
  const auto& sel = pDoc->GetSelectionManager()->GetSelection();

  if (m_DocsSubscribed[pDoc].m_bSubscribed == false)
  {
    m_DocsSubscribed[pDoc].m_bSubscribed = true;
    pDoc->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezVisualizerManager::StructureEventHandler, this));
  }

  ezVisualizerManagerEvent e;
  e.m_pSelection = &sel;
  e.m_pDocument = pDoc;

  m_Events.Broadcast(e);
}

void ezVisualizerManager::DocumentManagerEventHandler(const ezDocumentManager::Event& e)
{
  if (e.m_Type == ezDocumentManager::Event::Type::DocumentOpened)
  {
    e.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezVisualizerManager::SelectionEventHandler, this));
  }

  if (e.m_Type == ezDocumentManager::Event::Type::DocumentClosing)
  {
    ClearActiveVisualizers(e.m_pDocument);
  }

}

void ezVisualizerManager::StructureEventHandler(const ezDocumentObjectStructureEvent& event)
{
  if (!event.m_pDocument->GetSelectionManager()->IsSelectionEmpty() &&
      (event.m_EventType == ezDocumentObjectStructureEvent::Type::AfterObjectAdded ||
       event.m_EventType == ezDocumentObjectStructureEvent::Type::AfterObjectRemoved))
  {
    const auto& sel = event.m_pDocument->GetSelectionManager()->GetSelection();

    ezVisualizerManagerEvent e;
    e.m_pSelection = &sel;
    e.m_pDocument = event.m_pDocument;

    m_Events.Broadcast(e);
  }
}
