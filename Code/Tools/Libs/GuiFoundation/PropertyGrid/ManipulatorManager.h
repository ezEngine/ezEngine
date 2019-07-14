#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Configuration/Singleton.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Document/DocumentManager.h>

class ezManipulatorAttribute;
struct ezPhantomRttiManagerEvent;
struct ezSelectionManagerEvent;

struct EZ_GUIFOUNDATION_DLL ezManipulatorManagerEvent
{
  const ezDocument* m_pDocument;
  const ezManipulatorAttribute* m_pManipulator;
  const ezHybridArray<ezPropertySelection, 8>* m_pSelection;
  bool m_bHideManipulators;
};

class EZ_GUIFOUNDATION_DLL ezManipulatorManager
{
  EZ_DECLARE_SINGLETON(ezManipulatorManager);

public:

  ezManipulatorManager();
  ~ezManipulatorManager();

  const ezManipulatorAttribute* GetActiveManipulator(const ezDocument* pDoc, const ezHybridArray<ezPropertySelection, 8>*& out_Selection) const;

  void SetActiveManipulator(const ezDocument* pDoc, const ezManipulatorAttribute* pManipulator, const ezHybridArray<ezPropertySelection, 8>& selection);

  void ClearActiveManipulator(const ezDocument* pDoc);

  ezEvent<const ezManipulatorManagerEvent&> m_Events;

  void HideActiveManipulator(const ezDocument* pDoc, bool bHide);
  void ToggleHideActiveManipulator(const ezDocument* pDoc);

private:
  struct Data
  {
    Data()
    {
      m_pAttribute = nullptr;
      m_bHideManipulators = false;
    }

    const ezManipulatorAttribute* m_pAttribute;
    ezHybridArray<ezPropertySelection, 8> m_Selection;
    bool m_bHideManipulators;
  };

  void InternalSetActiveManipulator(const ezDocument* pDoc, const ezManipulatorAttribute* pManipulator, const ezHybridArray<ezPropertySelection, 8>& selection, bool bUnhide);

  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void SelectionEventHandler(const ezSelectionManagerEvent& e);

  void TransferToCurrentSelection(const ezDocument* pDoc);

  void PhantomTypeManagerEventHandler(const ezPhantomRttiManagerEvent& e);
  void DocumentManagerEventHandler(const ezDocumentManager::Event& e);
  
  ezMap<const ezDocument*, Data> m_ActiveManipulator;
};

