#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Configuration/Singleton.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

class ezManipulatorAttribute;
struct ezPhantomRttiManagerEvent;

struct ezManipulatorManagerEvent
{
  const ezDocument* m_pDocument;
  const ezManipulatorAttribute* m_pManipulator;
  const ezHybridArray<ezQtPropertyWidget::Selection, 8>* m_pSelection;
};

class EZ_GUIFOUNDATION_DLL ezManipulatorManager
{
  EZ_DECLARE_SINGLETON(ezManipulatorManager);

public:

  ezManipulatorManager();
  ~ezManipulatorManager();

  const ezManipulatorAttribute* GetActiveManipulator(const ezDocument* pDoc, const ezHybridArray<ezQtPropertyWidget::Selection, 8>*& out_Selection) const;

  void SetActiveManipulator(const ezDocument* pDoc, const ezManipulatorAttribute* pManipulator, const ezHybridArray<ezQtPropertyWidget::Selection, 8>& selection);

  ezEvent<const ezManipulatorManagerEvent&> m_Events;

private:
  struct Data
  {
    const ezManipulatorAttribute* m_pAttribute;
    ezHybridArray<ezQtPropertyWidget::Selection, 8> m_Selection;
  };

  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void PhantomTypeManagerEventHandler(const ezPhantomRttiManagerEvent& e);

  ezMap<const ezDocument*, Data> m_ActiveManipulator;
};

