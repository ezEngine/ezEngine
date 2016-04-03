#include <GuiFoundation/PCH.h>
#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <ToolsFoundation/Document/Document.h>

EZ_IMPLEMENT_SINGLETON(ezManipulatorManager);

EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, ManipulatorManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    EZ_DEFAULT_NEW(ezManipulatorManager);
  }

  ON_CORE_SHUTDOWN
  {
    if (ezManipulatorManager::GetSingleton())
    {
      auto ptr = ezManipulatorManager::GetSingleton();
      EZ_DEFAULT_DELETE(ptr);
    }
  }

EZ_END_SUBSYSTEM_DECLARATION

ezManipulatorManager::ezManipulatorManager()
  : m_SingletonRegistrar(this)
{
  ezPhantomRttiManager::s_Events.AddEventHandler(ezMakeDelegate(&ezManipulatorManager::PhantomTypeManagerEventHandler, this));
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezManipulatorManager::DocumentManagerEventHandler, this));
}

ezManipulatorManager::~ezManipulatorManager()
{
  ezPhantomRttiManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezManipulatorManager::PhantomTypeManagerEventHandler, this));
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezManipulatorManager::DocumentManagerEventHandler, this));
}

const ezManipulatorAttribute* ezManipulatorManager::GetActiveManipulator(const ezDocument* pDoc, const ezHybridArray<ezQtPropertyWidget::Selection, 8>*& out_Selection) const
{
  out_Selection = nullptr;
  auto it = m_ActiveManipulator.Find(pDoc);

  if (it.IsValid())
  {
    out_Selection = &(it.Value().m_Selection);

    return it.Value().m_pAttribute;
  }

  return nullptr;
}

void ezManipulatorManager::SetActiveManipulator(const ezDocument* pDoc, const ezManipulatorAttribute* pManipulator, const ezHybridArray<ezQtPropertyWidget::Selection, 8>& selection)
{
  bool existed = false;
  auto it = m_ActiveManipulator.FindOrAdd(pDoc, &existed);

  it.Value().m_pAttribute = pManipulator;
  it.Value().m_Selection = selection;

  if (!existed)
  {
    pDoc->GetObjectManager()->m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezManipulatorManager::StructureEventHandler, this));
    pDoc->GetSelectionManager()->m_Events.AddEventHandler(ezMakeDelegate(&ezManipulatorManager::SelectionEventHandler, this));
  }

  ezManipulatorManagerEvent e;
  e.m_pDocument = pDoc;
  e.m_pManipulator = pManipulator;
  e.m_pSelection = &it.Value().m_Selection;

  m_Events.Broadcast(e);
}

void ezManipulatorManager::ClearActiveManipulator(const ezDocument* pDoc)
{
  ezHybridArray<ezQtPropertyWidget::Selection, 8> clearSel;

  SetActiveManipulator(pDoc, nullptr, clearSel);
}

void ezManipulatorManager::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  if (e.m_EventType == ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved)
  {
    auto pDoc = e.m_pObject->GetDocumentObjectManager()->GetDocument();
    auto it = m_ActiveManipulator.Find(pDoc);

    if (it.IsValid())
    {
      for (auto& sel : it.Value().m_Selection)
      {
        if (sel.m_pObject == e.m_pObject)
        {
          it.Value().m_Selection.Remove(sel);
          SetActiveManipulator(pDoc, it.Value().m_pAttribute, it.Value().m_Selection);
          return;
        }
      }
    }
  }
}

void ezManipulatorManager::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  auto& data = m_ActiveManipulator[e.m_pDocument];
  auto pAttribute = data.m_pAttribute;

  if (pAttribute == nullptr)
    return;

  ezHybridArray<ezQtPropertyWidget::Selection, 8> newSelection;

  const auto& selection = e.m_pDocument->GetSelectionManager()->GetSelection();

  for (ezUInt32 i = 0; i < selection.GetCount(); ++i)
  {
    const auto& children = selection[i]->GetChildren();

    for (const auto& child : children)
    {
      const auto& OtherAttributes = child->GetTypeAccessor().GetType()->GetAttributes();

      for (const auto pOtherAttr : OtherAttributes)
      {
        if (pOtherAttr->IsInstanceOf(pAttribute->GetDynamicRTTI()))
        {
          ezManipulatorAttribute* pOtherManip = static_cast<ezManipulatorAttribute*>(pOtherAttr);

          if (pOtherManip->m_sProperty1 == pAttribute->m_sProperty1 &&
              pOtherManip->m_sProperty2 == pAttribute->m_sProperty2 &&
              pOtherManip->m_sProperty3 == pAttribute->m_sProperty3 &&
              pOtherManip->m_sProperty4 == pAttribute->m_sProperty4)
          {
            auto& newItem = newSelection.ExpandAndGetRef();
            newItem.m_pObject = child;
          }
        }
      }
    }
  }

  SetActiveManipulator(e.m_pDocument, pAttribute, newSelection);
}

void ezManipulatorManager::PhantomTypeManagerEventHandler(const ezPhantomRttiManagerEvent& e)
{
  if (e.m_Type == ezPhantomRttiManagerEvent::Type::TypeChanged || e.m_Type == ezPhantomRttiManagerEvent::Type::TypeRemoved)
  {
    ezHybridArray<ezQtPropertyWidget::Selection, 8> clearSel;

    for (auto it = m_ActiveManipulator.GetIterator(); it.IsValid(); ++it)
    {
      SetActiveManipulator(it.Key(), nullptr, clearSel);
    }
  }
}

void ezManipulatorManager::DocumentManagerEventHandler(const ezDocumentManager::Event& e)
{
  if (e.m_Type == ezDocumentManager::Event::Type::DocumentClosing)
  {
    ClearActiveManipulator(e.m_pDocument);

    e.m_pDocument->GetObjectManager()->m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezManipulatorManager::StructureEventHandler, this));
    m_ActiveManipulator.Remove(e.m_pDocument);
  }

}
