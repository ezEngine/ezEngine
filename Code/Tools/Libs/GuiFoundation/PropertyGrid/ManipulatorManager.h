#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Document/DocumentManager.h>

class ezManipulatorAttribute;
struct ezPhantomRttiManagerEvent;
struct ezSelectionManagerEvent;

/// Broadcast by ezManipulatorManager whenever the active manipulator or its selection changes.
///
/// If m_pManipulator is nullptr or m_bHideManipulators is true, any active gizmos should be
/// torn down. Listeners are responsible for updating or recreating their gizmos accordingly.
struct EZ_GUIFOUNDATION_DLL ezManipulatorManagerEvent
{
  const ezDocument* m_pDocument = nullptr;
  const ezManipulatorAttribute* m_pManipulator = nullptr;
  const ezHybridArray<ezPropertySelection, 8>* m_pSelection = nullptr;
  bool m_bHideManipulators = false;
};

/// Singleton that tracks which manipulator is active per document.
///
/// A manipulator becomes active when the property grid detects a focused property that carries
/// an ezManipulatorAttribute. The manager stores one active attribute and a corresponding
/// object selection per document and broadcasts m_Events whenever the state changes.
///
/// On selection changes the manager automatically re-evaluates which objects in the current
/// selection carry the same attribute and updates the stored selection accordingly. The
/// document type controls whether the manager looks at selected objects directly or at their
/// children via GetManipulatorSearchStrategy().
///
/// Hiding a manipulator (HideActiveManipulator / ToggleHideActiveManipulator) suppresses the
/// gizmo without clearing the active attribute, so it can be restored when un-hidden.
class EZ_GUIFOUNDATION_DLL ezManipulatorManager
{
  EZ_DECLARE_SINGLETON(ezManipulatorManager);

public:
  ezManipulatorManager();
  ~ezManipulatorManager();

  /// Returns the currently active manipulator attribute and the associated selection for the given document.
  /// Returns nullptr and sets out_pSelection to nullptr when no manipulator is active.
  const ezManipulatorAttribute* GetActiveManipulator(const ezDocument* pDoc, const ezHybridArray<ezPropertySelection, 8>*& out_pSelection) const;

  /// Sets the active manipulator for a document and broadcasts the change.
  /// Also resets the hidden flag so the gizmo becomes visible.
  void SetActiveManipulator(const ezDocument* pDoc, const ezManipulatorAttribute* pManipulator, const ezArrayPtr<ezPropertySelection>& selection);

  /// Deactivates the manipulator for the given document and broadcasts the change.
  void ClearActiveManipulator(const ezDocument* pDoc);

  /// Fired whenever the active manipulator, its selection, or the hidden state changes for any document.
  ezCopyOnBroadcastEvent<const ezManipulatorManagerEvent&> m_Events;

  /// Sets the hidden flag without deactivating the manipulator. Gizmos are hidden when bHide is true.
  void HideActiveManipulator(const ezDocument* pDoc, bool bHide);

  /// Toggles the hidden flag for the active manipulator of the given document.
  void ToggleHideActiveManipulator(const ezDocument* pDoc);

  /// Cycles through the manipulators available on the last selected object.
  ///
  /// If no manipulator is active, activates the first available one. If the currently active
  /// manipulator is not the last in the list, activates the next one. If it is the last,
  /// clears the active manipulator.
  void CycleActiveManipulator(const ezDocument* pDoc);

private:
  struct Data
  {
    const ezManipulatorAttribute* m_pAttribute = nullptr;
    ezHybridArray<ezPropertySelection, 8> m_Selection;
    bool m_bHideManipulators = false;
  };

  void InternalSetActiveManipulator(const ezDocument* pDoc, const ezManipulatorAttribute* pManipulator, const ezArrayPtr<ezPropertySelection>& selection, bool bUnhide);

  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void SelectionEventHandler(const ezSelectionManagerEvent& e);

  void TransferToCurrentSelection(const ezDocument* pDoc);

  void PhantomTypeManagerEventHandler(const ezPhantomRttiManagerEvent& e);
  void DocumentManagerEventHandler(const ezDocumentManager::Event& e);

  ezMap<const ezDocument*, Data> m_ActiveManipulator;
};
