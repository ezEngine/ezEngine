#pragma once

#include <EditorFramework/InputContexts/SelectionContext.h>

class ezSceneSelectionContext : public ezSelectionContext
{
public:
  ezSceneSelectionContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView, const ezCamera* pCamera);

protected:
  virtual void OpenDocumentForPickedObject(const ezObjectPickingResult& res) const override;
  virtual void SelectPickedObject(const ezObjectPickingResult& res, bool bToggle, bool bDirect) const override;

  ezUuid FindLayerByObject(ezUuid objectGuid) const;
};
