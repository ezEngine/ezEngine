#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Configuration/Singleton.h>
#include <ToolsFoundation/Document/DocumentManager.h>

struct ezSelectionManagerEvent;
class ezDocumentObject;
class ezVisualizerAttribute;

struct EZ_GUIFOUNDATION_DLL ezVisualizerManagerEvent
{
  const ezDocument* m_pDocument;
  const ezDeque<const ezDocumentObject*>* m_pSelection;
};

class EZ_GUIFOUNDATION_DLL ezVisualizerManager
{
  EZ_DECLARE_SINGLETON(ezVisualizerManager);

public:

  ezVisualizerManager();
  ~ezVisualizerManager();

  ezEvent<const ezVisualizerManagerEvent&> m_Events;

private:

  void ClearActiveVisualizers(const ezDocument* pDoc);
  void SelectionEventHandler(const ezSelectionManagerEvent& e);
  void DocumentManagerEventHandler(const ezDocumentManager::Event& e);
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);

  ezSet<const ezDocument*> m_DocsSubscribed;
};

