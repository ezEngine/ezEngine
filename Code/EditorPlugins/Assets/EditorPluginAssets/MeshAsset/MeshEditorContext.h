#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

#include <EditorFramework/InputContexts/EditorInputContext.h>

/// \brief Input context for the mesh asset editor that handles Ctrl+Middle click to open the material at the cursor.
///
/// Ctrl+Middle click fires a picking query to determine the material slot under the cursor,
/// then opens the corresponding material document. Used only in the mesh asset preview.
class EZ_EDITORPLUGINASSETS_DLL ezMeshEditorInputContext : public ezEditorInputContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMeshEditorInputContext, ezEditorInputContext);

public:
  ezMeshEditorInputContext(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView);

protected:
  virtual void OnSetOwner(ezQtEngineDocumentWindow* pOwnerWindow, ezQtEngineViewWidget* pOwnerView) override {}
  virtual ezEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
};
