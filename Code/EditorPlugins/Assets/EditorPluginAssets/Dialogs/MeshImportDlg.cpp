#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/Dialogs/MeshImportDlg.moc.h>
#include <QFileDialog>

ezMeshImportDlg::ezMeshImportDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);
}

void ezMeshImportDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  CurrentAsset->setText(ezMakeQString(m_sTitle));
  Skeleton->setVisible(m_bShowAnimMeshOptions);
  Animations->setVisible(m_bShowAnimMeshOptions);
  ApplyToAll->setChecked(m_bApplyToAll);
  ImportAnimClips->setChecked(m_bImportAnimationClips);
  ReuseSkeleton->setChecked(m_bReuseExistingSkeleton);
  UseSharedMaterials->setChecked(m_bUseSharedMaterials);

  UpdateUI();
}

void ezMeshImportDlg::UpdateUI()
{

  BrowseMaterialsFolder->setEnabled(m_bUseSharedMaterials);
  MaterialsFolder->setEnabled(m_bUseSharedMaterials);
  MaterialsFolder->setReadOnly(true);
  Materials->setChecked(m_bCreateMaterials);
  SkeletonAsset->setEnabled(m_bReuseExistingSkeleton);
  BrowseSkeleton->setEnabled(m_bReuseExistingSkeleton);

  ezStringBuilder sRelPath = m_sSharedMaterialsFolderAbs;
  if (ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sRelPath))
  {
    MaterialsFolder->setText(ezMakeQString(sRelPath));
  }

  bool ok = true;
  if (m_bCreateMaterials && m_bUseSharedMaterials)
  {
    if (!ezOSFile::ExistsDirectory(m_sSharedMaterialsFolderAbs))
    {
      ok = false;
    }
  }

  if (!m_bCreateMaterials)
  {
    MaterialsFolder->setText("No material assets will be created.");
  }
  else if (!m_bUseSharedMaterials)
  {
    MaterialsFolder->setText("Creating material assets in '_data' folder besides asset.");
  }

  if (m_bShowAnimMeshOptions)
  {
    if (!m_bReuseExistingSkeleton)
    {
      SkeletonAsset->setText("A new skeleton asset will be created.");
    }
    else
    {
      if (!m_SharedSkeleton.IsValid())
      {
        m_sSharedSkeleton.Clear();
        ok = false;

        SkeletonAsset->setText("Browse for an existing skeleton asset ->");
      }
      else
      {
        if (m_sSharedSkeleton.IsEmpty())
        {
          if (auto pAsset = ezAssetCurator::GetSingleton()->GetSubAsset(m_SharedSkeleton))
          {
            m_sSharedSkeleton = pAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
          }
        }

        if (m_sSharedSkeleton.IsEmpty())
        {
          ok = false;
          SkeletonAsset->setText("Invalid skeleton asset.");
        }
        else
        {
          SkeletonAsset->setText(ezMakeQString(m_sSharedSkeleton));
        }
      }
    }
  }

  Buttons->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(ok);
}

void ezMeshImportDlg::on_Buttons_accepted()
{
  m_bCreateMaterials = Materials->isChecked();
  m_bImportAnimationClips = ImportAnimClips->isChecked();
  m_bApplyToAll = ApplyToAll->isChecked();

  accept();
}

void ezMeshImportDlg::on_Buttons_rejected()
{
  reject();
}

void ezMeshImportDlg::on_BrowseMaterialsFolder_clicked()
{
  ezStringBuilder sPath = m_sSharedMaterialsFolderAbs;

  if (sPath.IsEmpty())
  {
    sPath = ezToolsProject::GetSingleton()->GetProjectDirectory();
  }

  if (!ezQtEditorApp::GetSingleton()->MakeParentDataDirectoryRelativePathAbsolute(sPath, true))
  {
    sPath.Clear();
  }

  QString sSelectedPath = QFileDialog::getExistingDirectory(this, "Select Folder", ezMakeQString(sPath));
  if (sSelectedPath.isEmpty())
    return;

  ezStringBuilder sRelPath = sSelectedPath.toUtf8().data();

  if (!ezQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sRelPath))
  {
    ezQtUiServices::GetSingleton()->MessageBoxInformation("The select path isn't in any of the project's data directories.\n\nPlease select another folder.");
    return;
  }

  m_sSharedMaterialsFolderAbs = sSelectedPath.toUtf8().data();

  UpdateUI();
}

void ezMeshImportDlg::on_BrowseSkeleton_clicked()
{
  ezQtAssetBrowserDlg dlg(this, ezUuid::MakeInvalid(), "CompatibleAsset_Mesh_Skeleton", "Select Skeleton");
  if (dlg.exec() != 0)
  {
    if (dlg.GetSelectedAssetGuid().IsValid())
    {
      m_SharedSkeleton = dlg.GetSelectedAssetGuid();
      m_sSharedSkeleton.Clear();
    }

    UpdateUI();
  }
}

void ezMeshImportDlg::on_UseSharedMaterials_clicked(bool)
{
  m_bUseSharedMaterials = UseSharedMaterials->isChecked();

  UpdateUI();
}

void ezMeshImportDlg::on_Materials_clicked(bool)
{
  m_bCreateMaterials = Materials->isChecked();

  UpdateUI();
}

void ezMeshImportDlg::on_ReuseSkeleton_clicked(bool)
{
  m_bReuseExistingSkeleton = ReuseSkeleton->isChecked();

  UpdateUI();
}
