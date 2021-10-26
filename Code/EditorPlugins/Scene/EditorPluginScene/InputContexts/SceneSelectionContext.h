#pragma once

#include <EditorFramework/InputContexts/SelectionContext.h>

/// \brief Custom selection context for the scene to allow switching the active layer if an object is clicked that is in a different layer then the active one.
class ezSceneSelectionContext : public ezSelectionContext
{
public:
  ezSceneSelectionContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, const ezCamera* pCamera);

protected:
  virtual void OpenDocumentForPickedObject(const ezObjectPickingResult& res) const override;
  virtual void SelectPickedObject(const ezObjectPickingResult& res, bool bToggle, bool bDirect) const override;

  ezUuid FindLayerByObject(ezUuid objectGuid, const ezDocumentObject*& out_pObject) const;
};
