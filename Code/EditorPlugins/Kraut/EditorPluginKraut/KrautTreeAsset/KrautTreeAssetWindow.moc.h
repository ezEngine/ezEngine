#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class ezQtOrbitCamViewWidget;
class ezQtPropertyGridWidget;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;

/// Editor document window for a Kraut tree asset.
///
/// The window listens to property changes and sends redraw messages to the engine process
/// to keep the preview in sync with the current asset state.
class ezQtKrautTreeAssetDocumentWindow : public ezQtEngineDocumentWindow
{
  Q_OBJECT

public:
  ezQtKrautTreeAssetDocumentWindow(ezAssetDocument* pDocument);
  ~ezQtKrautTreeAssetDocumentWindow();

  ezKrautTreeAssetDocument* GetKrautDocument() const
  {
    return static_cast<ezKrautTreeAssetDocument*>(GetDocument());
  }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const ezEditorEngineDocumentMsg* pMsg) override;
  void PropertyEventHandler(const ezDocumentObjectPropertyEvent& e);

private Q_SLOTS:
  void onBranchTypeSelected(int index);
  void onLodSelected(int index);
  void onAutoLodToggled(bool bChecked);
  void onAddLod();
  void onDeleteLod();
  void onImportKrautFile();
  void onCopyBranchType();
  void onPasteBranchType();
  void onCopyLod();
  void onPasteLod();

private:
  void SendRedrawMsg();
  void QueryObjectBBox(ezInt32 iPurpose = 0);
  void UpdatePreview();
  void RestoreResource();
  void ImportKrautFile();
  void RebuildBranchTypeCombo();
  void RebuildLodCombo();
  void ApplyLodPreset(int iPresetIndex);
  ezDocumentObject* GetCurrentBranchTypeObject() const;
  ezDocumentObject* GetCurrentLodObject() const;
  void CopyObjectToClipboard(const ezDocumentObject* pObject, const char* szMimeType);
  void PasteObjectFromClipboard(ezDocumentObject* pObject, const char* szMimeType, const char* szTransactionName);

  ezEngineViewConfig m_ViewConfig;
  ezQtOrbitCamViewWidget* m_pViewWidget = nullptr;
  ezKrautTreeAssetDocument* m_pAssetDoc = nullptr;
  ezQtPropertyGridWidget* m_pBranchProps = nullptr;
  ezQtPropertyGridWidget* m_pLodProps = nullptr;
  QComboBox* m_pBranchTypeCombo = nullptr;
  QComboBox* m_pLodCombo = nullptr;
  QPushButton* m_pAddLodButton = nullptr;
  QPushButton* m_pDeleteLodButton = nullptr;
  QCheckBox* m_pAutoLodCheckbox = nullptr;
  int m_iCurrentLodIndex = 0; ///< Index of the LOD shown in the preview; -1 = full detail (no reduction), 0-4 = forced LOD index.

  QLabel* m_pStatsBones = nullptr;
  QLabel* m_pStatsTotal = nullptr;
  QLabel* m_pStatsBranch = nullptr;
  QLabel* m_pStatsFrond = nullptr;
  QLabel* m_pStatsLeaves = nullptr;
};
