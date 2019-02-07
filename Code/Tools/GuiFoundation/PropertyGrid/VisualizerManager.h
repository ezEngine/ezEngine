#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>
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

  void SetVisualizersActive(const ezDocument* pDoc, bool bActive);

  ezEvent<const ezVisualizerManagerEvent&> m_Events;

private:

  void SelectionEventHandler(const ezSelectionManagerEvent& e);
  void DocumentManagerEventHandler(const ezDocumentManager::Event& e);
  void StructureEventHandler(const ezDocumentObjectStructureEvent& e);
  void SendEventToRecreateVisualizers(const ezDocument* pDoc);

  struct DocData
  {
    bool m_bSubscribed;
    bool m_bActivated;

    DocData()
    {
      m_bSubscribed = false;
      m_bActivated = true;
    }
  };

  ezMap<const ezDocument*, DocData> m_DocsSubscribed;
};

