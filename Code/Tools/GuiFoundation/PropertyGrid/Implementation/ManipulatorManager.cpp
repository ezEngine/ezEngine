#include <PCH.h>
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

const ezManipulatorAttribute* ezManipulatorManager::GetActiveManipulator(const ezDocument* pDoc, const ezHybridArray<ezPropertySelection, 8>*& out_Selection) const
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

void ezManipulatorManager::InternalSetActiveManipulator(const ezDocument* pDoc, const ezManipulatorAttribute* pManipulator, const ezHybridArray<ezPropertySelection, 8>& selection, bool bUnhide)
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

  auto& data = m_ActiveManipulator[pDoc];

  if (bUnhide)
  {
    data.m_bHideManipulators = false;
  }

  ezManipulatorManagerEvent e;
  e.m_bHideManipulators = data.m_bHideManipulators;
  e.m_pDocument = pDoc;
  e.m_pManipulator = pManipulator;
  e.m_pSelection = &data.m_Selection;

  m_Events.Broadcast(e);
}


void ezManipulatorManager::SetActiveManipulator(const ezDocument* pDoc, const ezManipulatorAttribute* pManipulator, const ezHybridArray<ezPropertySelection, 8>& selection)
{
  InternalSetActiveManipulator(pDoc, pManipulator, selection, true);
}

void ezManipulatorManager::ClearActiveManipulator(const ezDocument* pDoc)
{
  ezHybridArray<ezPropertySelection, 8> clearSel;

  InternalSetActiveManipulator(pDoc, nullptr, clearSel, false);
}

void ezManipulatorManager::HideActiveManipulator(const ezDocument* pDoc, bool bHide)
{
  auto it = m_ActiveManipulator.Find(pDoc);

  if (it.IsValid() && it.Value().m_bHideManipulators != bHide)
  {
    it.Value().m_bHideManipulators = bHide;

    if (bHide)
    {
      ezHybridArray<ezPropertySelection, 8> clearSel;
      InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, clearSel, false);
    }
    else
    {
      TransferToCurrentSelection(pDoc);
    }
  }
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
          InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, it.Value().m_Selection, false);
          return;
        }
      }
    }
  }
}

void ezManipulatorManager::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  TransferToCurrentSelection(e.m_pDocument);
}

void ezManipulatorManager::TransferToCurrentSelection(const ezDocument* pDoc)
{
  auto& data = m_ActiveManipulator[pDoc];
  auto pAttribute = data.m_pAttribute;

  if (pAttribute == nullptr)
    return;

  if (data.m_bHideManipulators)
    return;

  ezHybridArray<ezPropertySelection, 8> newSelection;

  const auto& selection = pDoc->GetSelectionManager()->GetSelection();

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

  InternalSetActiveManipulator(pDoc, pAttribute, newSelection, false);
}

void ezManipulatorManager::PhantomTypeManagerEventHandler(const ezPhantomRttiManagerEvent& e)
{
  if (e.m_Type == ezPhantomRttiManagerEvent::Type::TypeChanged || e.m_Type == ezPhantomRttiManagerEvent::Type::TypeRemoved)
  {
    for (auto it = m_ActiveManipulator.GetIterator(); it.IsValid(); ++it)
    {
      ClearActiveManipulator(it.Key());
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
