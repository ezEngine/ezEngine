#include <PCH.h>
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

  SendEventToRecreateVisualizers(pDoc);
}

void ezVisualizerManager::SelectionEventHandler(const ezSelectionManagerEvent& event)
{
  if (!m_DocsSubscribed[event.m_pDocument].m_bActivated)
    return;

  SendEventToRecreateVisualizers(event.m_pDocument);
}

void ezVisualizerManager::SendEventToRecreateVisualizers(const ezDocument* pDoc)
{
  if (m_DocsSubscribed[pDoc].m_bSubscribed == false)
  {
    m_DocsSubscribed[pDoc].m_bSubscribed = true;
    pDoc->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezVisualizerManager::StructureEventHandler, this));
  }

  if (m_DocsSubscribed[pDoc].m_bActivated)
  {
    const auto& sel = pDoc->GetSelectionManager()->GetSelection();

    ezVisualizerManagerEvent e;
    e.m_pSelection = &sel;
    e.m_pDocument = pDoc;
    m_Events.Broadcast(e);
  }
  else
  {
    ezDeque<const ezDocumentObject*> sel;

    ezVisualizerManagerEvent e;
    e.m_pSelection = &sel;
    e.m_pDocument = pDoc;

    m_Events.Broadcast(e);
  }
}

void ezVisualizerManager::DocumentManagerEventHandler(const ezDocumentManager::Event& e)
{
  if (e.m_Type == ezDocumentManager::Event::Type::DocumentOpened)
  {
    e.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezVisualizerManager::SelectionEventHandler, this));
  }

  if (e.m_Type == ezDocumentManager::Event::Type::DocumentClosing)
  {
    SetVisualizersActive(e.m_pDocument, false);
  }
}

void ezVisualizerManager::StructureEventHandler(const ezDocumentObjectStructureEvent& event)
{
  if (!m_DocsSubscribed[event.m_pDocument].m_bActivated)
    return;

  if (!event.m_pDocument->GetSelectionManager()->IsSelectionEmpty() &&
      (event.m_EventType == ezDocumentObjectStructureEvent::Type::AfterObjectAdded ||
       event.m_EventType == ezDocumentObjectStructureEvent::Type::AfterObjectRemoved))
  {
    SendEventToRecreateVisualizers(event.m_pDocument);
  }
}
