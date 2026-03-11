#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/PropertyGrid/ManipulatorManager.h>
#include <ToolsFoundation/Document/Document.h>

EZ_IMPLEMENT_SINGLETON(ezManipulatorManager);

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, ManipulatorManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    EZ_DEFAULT_NEW(ezManipulatorManager);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    if (ezManipulatorManager::GetSingleton())
    {
      auto ptr = ezManipulatorManager::GetSingleton();
      EZ_DEFAULT_DELETE(ptr);
    }
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

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

const ezManipulatorAttribute* ezManipulatorManager::GetActiveManipulator(const ezDocument* pDoc, const ezHybridArray<ezPropertySelection, 8>*& out_pSelection) const
{
  out_pSelection = nullptr;
  auto it = m_ActiveManipulator.Find(pDoc);

  if (it.IsValid())
  {
    out_pSelection = &(it.Value().m_Selection);

    return it.Value().m_pAttribute;
  }

  return nullptr;
}

void ezManipulatorManager::InternalSetActiveManipulator(const ezDocument* pDoc, const ezManipulatorAttribute* pManipulator, const ezArrayPtr<ezPropertySelection>& selection, bool bUnhide)
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


void ezManipulatorManager::SetActiveManipulator(const ezDocument* pDoc, const ezManipulatorAttribute* pManipulator, const ezArrayPtr<ezPropertySelection>& selection)
{
  InternalSetActiveManipulator(pDoc, pManipulator, selection, true);
}

void ezManipulatorManager::ClearActiveManipulator(const ezDocument* pDoc)
{
  ezTempHybridArray<ezPropertySelection, 8> clearSel;

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
      ezTempHybridArray<ezPropertySelection, 8> clearSel;
      InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, clearSel, false);
    }
    else
    {
      TransferToCurrentSelection(pDoc);
    }
  }
}

void ezManipulatorManager::ToggleHideActiveManipulator(const ezDocument* pDoc)
{
  auto it = m_ActiveManipulator.Find(pDoc);

  if (it.IsValid())
  {
    it.Value().m_bHideManipulators = !it.Value().m_bHideManipulators;

    if (it.Value().m_bHideManipulators)
    {
      ezTempHybridArray<ezPropertySelection, 8> clearSel;
      InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, clearSel, false);
    }
    else
    {
      TransferToCurrentSelection(pDoc);
    }
  }
}

void ezManipulatorManager::CycleActiveManipulator(const ezDocument* pDoc)
{
  const ezDocumentObject* pCurrentObject = pDoc->GetSelectionManager()->GetCurrentObject();
  if (pCurrentObject == nullptr)
    return;

  EZ_ASSERT_DEV(pDoc->GetManipulatorSearchStrategy() != ezManipulatorSearchStrategy::None,
    "The document type '{}' has to override the function 'GetManipulatorSearchStrategy()'", pDoc->GetDynamicRTTI()->GetTypeName());

  // Collect available manipulator attributes from the last selected object (or its children).
  // Deduplicates by type + primary property so that e.g. multiple ezSplineNodeComponent children
  // that share the same ezSplineManipulatorAttribute only contribute one entry.
  ezTempHybridArray<const ezManipulatorAttribute*, 8> available;

  auto collectManipulators = [&](const ezDocumentObject* pObj)
  {
    // Walk the full type hierarchy so attributes on base classes are included.
    for (const ezRTTI* pRtti = pObj->GetTypeAccessor().GetType(); pRtti != nullptr; pRtti = pRtti->GetParentType())
    {
      for (const auto* pAttr : pRtti->GetAttributes())
      {
        if (!pAttr->GetDynamicRTTI()->IsDerivedFrom<ezManipulatorAttribute>())
          continue;

        const ezManipulatorAttribute* pManipAttr = static_cast<const ezManipulatorAttribute*>(pAttr);

        // Skip duplicates (same type and primary property)
        bool bAlreadyAdded = false;
        for (const auto* pExisting : available)
        {
          if (pExisting->GetDynamicRTTI() == pManipAttr->GetDynamicRTTI() &&
              pExisting->m_sProperty1 == pManipAttr->m_sProperty1)
          {
            bAlreadyAdded = true;
            break;
          }
        }
        if (!bAlreadyAdded)
          available.PushBack(pManipAttr);
      }
    }
  };

  if (pDoc->GetManipulatorSearchStrategy() == ezManipulatorSearchStrategy::SelectedObject)
  {
    collectManipulators(pCurrentObject);
  }
  else if (pDoc->GetManipulatorSearchStrategy() == ezManipulatorSearchStrategy::ChildrenOfSelectedObject)
  {
    for (const auto* pChild : pCurrentObject->GetChildren())
      collectManipulators(pChild);
  }
  else
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  if (available.IsEmpty())
    return;

  // Find the index of the currently active manipulator
  const ezHybridArray<ezPropertySelection, 8>* pSel = nullptr;
  const ezManipulatorAttribute* pActive = GetActiveManipulator(pDoc, pSel);

  ezInt32 iCurrentIndex = -1;
  if (pActive != nullptr)
  {
    for (ezUInt32 i = 0; i < available.GetCount(); ++i)
    {
      if (available[i]->GetDynamicRTTI() == pActive->GetDynamicRTTI() &&
          available[i]->m_sProperty1 == pActive->m_sProperty1)
      {
        iCurrentIndex = static_cast<ezInt32>(i);
        break;
      }
    }
  }

  // If the last (or only) manipulator is currently active, clear and stop
  if (iCurrentIndex >= static_cast<ezInt32>(available.GetCount()) - 1)
  {
    ClearActiveManipulator(pDoc);
    return;
  }

  const ezManipulatorAttribute* pNext = available[iCurrentIndex + 1];

  // Build a selection for pNext by searching through the current editor selection
  ezTempHybridArray<ezPropertySelection, 8> newSelection;
  const auto& selection = pDoc->GetSelectionManager()->GetSelection();

  auto matchesNext = [&](const ezManipulatorAttribute* pManip) -> bool
  {
    return pManip->GetDynamicRTTI() == pNext->GetDynamicRTTI() &&
           pManip->m_sProperty1 == pNext->m_sProperty1 && pManip->m_sProperty2 == pNext->m_sProperty2 &&
           pManip->m_sProperty3 == pNext->m_sProperty3 && pManip->m_sProperty4 == pNext->m_sProperty4 &&
           pManip->m_sProperty5 == pNext->m_sProperty5 && pManip->m_sProperty6 == pNext->m_sProperty6;
  };

  // Returns true if pObj (or any of its base types) has a manipulator attribute matching pNext.
  auto hasMatchingManipulator = [&](const ezDocumentObject* pObj) -> bool
  {
    for (const ezRTTI* pRtti = pObj->GetTypeAccessor().GetType(); pRtti != nullptr; pRtti = pRtti->GetParentType())
    {
      for (const auto* pAttr : pRtti->GetAttributes())
      {
        if (pAttr->IsInstanceOf(pNext->GetDynamicRTTI()) && matchesNext(static_cast<const ezManipulatorAttribute*>(pAttr)))
          return true;
      }
    }
    return false;
  };

  if (pDoc->GetManipulatorSearchStrategy() == ezManipulatorSearchStrategy::SelectedObject)
  {
    for (const auto* pObj : selection)
    {
      if (hasMatchingManipulator(pObj))
        newSelection.ExpandAndGetRef().m_pObject = pObj;
    }
  }
  else if (pDoc->GetManipulatorSearchStrategy() == ezManipulatorSearchStrategy::ChildrenOfSelectedObject)
  {
    for (const auto* pObj : selection)
    {
      for (const auto* pChild : pObj->GetChildren())
      {
        if (hasMatchingManipulator(pChild))
        {
          newSelection.ExpandAndGetRef().m_pObject = pChild;
          break;
        }
      }
    }
  }
  else
  {
    EZ_ASSERT_NOT_IMPLEMENTED;
  }

  InternalSetActiveManipulator(pDoc, pNext, newSelection, true);
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
          it.Value().m_Selection.RemoveAndCopy(sel);
          InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, it.Value().m_Selection, false);
          return;
        }
      }
    }
  }

  if (e.m_EventType == ezDocumentObjectStructureEvent::Type::BeforeReset)
  {
    auto pDoc = e.m_pDocument;
    auto it = m_ActiveManipulator.Find(pDoc);

    if (it.IsValid())
    {
      for (auto& sel : it.Value().m_Selection)
      {
        it.Value().m_Selection.RemoveAndCopy(sel);
        InternalSetActiveManipulator(pDoc, it.Value().m_pAttribute, it.Value().m_Selection, false);
      }
    }
  }
}

void ezManipulatorManager::SelectionEventHandler(const ezSelectionManagerEvent& e)
{
  TransferToCurrentSelection(e.m_pDocument->GetMainDocument());
}

void ezManipulatorManager::TransferToCurrentSelection(const ezDocument* pDoc)
{
  auto& data = m_ActiveManipulator[pDoc];
  auto pAttribute = data.m_pAttribute;

  if (pAttribute == nullptr)
    return;

  if (data.m_bHideManipulators)
    return;

  ezTempHybridArray<ezPropertySelection, 8> newSelection;

  const auto& selection = pDoc->GetSelectionManager()->GetSelection();

  EZ_ASSERT_DEV(pDoc->GetManipulatorSearchStrategy() != ezManipulatorSearchStrategy::None, "The document type '{}' has to override the function 'GetManipulatorSearchStrategy()'", pDoc->GetDynamicRTTI()->GetTypeName());

  if (pDoc->GetManipulatorSearchStrategy() == ezManipulatorSearchStrategy::SelectedObject)
  {
    for (ezUInt32 i = 0; i < selection.GetCount(); ++i)
    {
      const auto& OtherAttributes = selection[i]->GetTypeAccessor().GetType()->GetAttributes();

      for (const auto pOtherAttr : OtherAttributes)
      {
        if (pOtherAttr->IsInstanceOf(pAttribute->GetDynamicRTTI()))
        {
          auto pOtherManip = static_cast<const ezManipulatorAttribute*>(pOtherAttr);

          if (pOtherManip->m_sProperty1 == pAttribute->m_sProperty1 && pOtherManip->m_sProperty2 == pAttribute->m_sProperty2 &&
              pOtherManip->m_sProperty3 == pAttribute->m_sProperty3 && pOtherManip->m_sProperty4 == pAttribute->m_sProperty4 &&
              pOtherManip->m_sProperty5 == pAttribute->m_sProperty5 && pOtherManip->m_sProperty6 == pAttribute->m_sProperty6)
          {
            auto& newItem = newSelection.ExpandAndGetRef();
            newItem.m_pObject = selection[i];
          }
        }
      }
    }
  }

  if (pDoc->GetManipulatorSearchStrategy() == ezManipulatorSearchStrategy::ChildrenOfSelectedObject)
  {
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
            auto pOtherManip = static_cast<const ezManipulatorAttribute*>(pOtherAttr);

            if (pOtherManip->m_sProperty1 == pAttribute->m_sProperty1 && pOtherManip->m_sProperty2 == pAttribute->m_sProperty2 &&
                pOtherManip->m_sProperty3 == pAttribute->m_sProperty3 && pOtherManip->m_sProperty4 == pAttribute->m_sProperty4 &&
                pOtherManip->m_sProperty5 == pAttribute->m_sProperty5 && pOtherManip->m_sProperty6 == pAttribute->m_sProperty6)
            {
              auto& newItem = newSelection.ExpandAndGetRef();
              newItem.m_pObject = child;
            }
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
    e.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezManipulatorManager::SelectionEventHandler, this));

    m_ActiveManipulator.Remove(e.m_pDocument);
  }
}
