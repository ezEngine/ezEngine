#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <EditorPluginAssets/ui_MeshImportDlg.h>
#include <QDialog>

class ezMeshImportDlg : public QDialog, public Ui_MeshImportDlg
{
  Q_OBJECT

public:
  ezMeshImportDlg(QWidget* pParent);

  ezString m_sTitle;
  ezString m_sSharedMaterialsFolderAbs;
  bool m_bApplyToAll = false;
  bool m_bUseSharedMaterials = false;
  bool m_bCreateMaterials = true;
  bool m_bShowAnimMeshOptions = false;
  bool m_bReuseExistingSkeleton = false;
  bool m_bImportAnimationClips = false;
  ezUuid m_SharedSkeleton;

private Q_SLOTS:
  void on_Buttons_accepted();
  void on_Buttons_rejected();
  void on_BrowseMaterialsFolder_clicked();
  void on_BrowseSkeleton_clicked();
  void on_UseSharedMaterials_clicked(bool);
  void on_Materials_clicked(bool);
  void on_ReuseSkeleton_clicked(bool);

private:
  virtual void showEvent(QShowEvent*) override;

  void UpdateUI();

  ezString m_sSharedSkeleton;
};
