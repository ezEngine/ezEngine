#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EnginePluginKraut/EnginePluginKrautDLL.h>
#include <KrautPlugin/Components/KrautTreeComponent.h>
#include <RendererCore/Meshes/MeshResource.h>

class ezObjectSelectionMsgToEngine;
class ezRenderContext;

/// Engine-process document context for a Kraut tree asset.
///
/// Runs inside the editor's engine process and owns the game world used for the asset preview.
/// It creates an ezKrautTreeComponent driven by the generator resource, handles messages from
/// the editor (seed changes, LOD selection, bounding-box queries), and periodically sends
/// LOD triangle statistics back so the editor window can display them.
class EZ_ENGINEPLUGINKRAUT_DLL ezKrautTreeContext : public ezEngineProcessDocumentContext
{
  EZ_ADD_DYNAMIC_REFLECTION(ezKrautTreeContext, ezEngineProcessDocumentContext);

public:
  ezKrautTreeContext();

  virtual void HandleMessage(const ezEditorEngineDocumentMsg* pMsg) override;
  const ezKrautGeneratorResourceHandle& GetResource() const { return m_hMainResource; }
  ezComponentHandle GetKrautComponentHandle() const { return m_hKrautComponent; }

protected:
  virtual void OnInitialize() override;

  virtual ezEngineProcessViewContext* CreateViewContext() override;
  virtual void DestroyViewContext(ezEngineProcessViewContext* pContext) override;
  virtual bool UpdateThumbnailViewContext(ezEngineProcessViewContext* pThumbnailViewContext) override;

private:
  void QuerySelectionBBox(const ezEditorEngineDocumentMsg* pMsg);
  /// Sends per-LOD triangle and bone counts to the editor document window.
  void SendLodStats(const ezUuid& documentGuid);

  /// Triangle and bone statistics for one LOD, sent to the editor for display in the stats labels.
  struct LodStats
  {
    ezInt8 m_iLodIndex = -2; ///< Index of the LOD these stats describe; -2 = not yet initialized.
    ezUInt16 m_uiNumBones = 0;
    ezUInt32 m_uiNumTrianglesTotal = 0;
    ezUInt32 m_uiNumTrianglesBranch = 0;
    ezUInt32 m_uiNumTrianglesFrond = 0;
    ezUInt32 m_uiNumTrianglesLeaf = 0;
  };

  ezGameObject* m_pMainObject;
  ezComponentHandle m_hKrautComponent;
  ezComponentHandle m_hWindComponent;
  ezKrautGeneratorResourceHandle m_hMainResource;
  ezMeshResourceHandle m_hPreviewMeshResource;
  ezUInt32 m_uiDisplayRandomSeed = 0xFFFFFFFF; ///< The seed currently shown in the preview; 0xFFFFFFFF means not yet set.
  LodStats m_LastSentLodStats;                 ///< Cached stats from the last SendLodStats() call, used to avoid redundant messages.
};
