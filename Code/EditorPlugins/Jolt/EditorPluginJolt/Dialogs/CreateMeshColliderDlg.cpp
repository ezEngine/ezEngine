#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginJolt/Dialogs/CreateMeshColliderDlg.moc.h>
#include <Foundation/IO/OSFile.h>
#include <QFileDialog>
#include <QPushButton>
#include <ToolsFoundation/Project/ToolsProject.h>

bool ezQtCreateMeshColliderDlg::s_bOpenAfterCreate = true;
ezEnum<ezMeshColliderKind> ezQtCreateMeshColliderDlg::s_LastKind = ezMeshColliderKind::Default;

ezQtCreateMeshColliderDlg::ezQtCreateMeshColliderDlg(const ezMeshColliderSource& source, QWidget* pParent)
  : ezQtDialog(pParent)
  , m_pSource(&source)
{
  Setup();

  UpdateSuggestedPath();
  UpdateInfo();
}

ezQtCreateMeshColliderDlg::ezQtCreateMeshColliderDlg(ezUInt32 uiMeshCount, QWidget* pParent)
  : ezQtDialog(pParent)
  , m_uiMeshCount(uiMeshCount)
{
  Setup();

  // with several meshes there is no one path to pick, each collider goes next to its own mesh
  ColliderPath->setEnabled(false);
  BrowseButton->setEnabled(false);
  m_bSettingPath = true;
  ColliderPath->setText(QLatin1String("<next to each mesh asset>"));
  m_bSettingPath = false;

  // the box stays available, it is only unticked by default for a large selection
  OpenAfterCreate->setText(QString("Open all %1 collision meshes after creating them").arg(uiMeshCount));
  OpenAfterCreate->setChecked(s_bOpenAfterCreate && uiMeshCount <= s_uiMaxAutoOpen);

  UpdateInfo();
}

void ezQtCreateMeshColliderDlg::Setup()
{
  setupUi(this);

  ColliderType->addItem("Convex Hull", QVariant((int)ezMeshColliderKind::ConvexHull));
  ColliderType->addItem("Triangle Mesh", QVariant((int)ezMeshColliderKind::TriangleMesh));
  ColliderType->setCurrentIndex(ColliderType->findData(QVariant((int)s_LastKind.GetValue())));

  OpenAfterCreate->setChecked(s_bOpenAfterCreate);
}

ezEnum<ezMeshColliderKind> ezQtCreateMeshColliderDlg::GetSelectedKind() const
{
  return (ezMeshColliderKind::Enum)ColliderType->currentData().toInt();
}

void ezQtCreateMeshColliderDlg::UpdateSuggestedPath()
{
  if (m_pSource == nullptr || !m_bPathIsSuggestion)
    return;

  const ezString sPath = ezMeshColliderCreator::MakeDisplayPath(
    ezMeshColliderCreator::SuggestColliderPath(*m_pSource, GetSelectedKind(), OverwriteExisting->isChecked()));

  // the change is our own, so the text handler must not treat it as the user typing a path
  m_bSettingPath = true;
  ColliderPath->setText(ezMakeQString(sPath));
  m_bSettingPath = false;
}

void ezQtCreateMeshColliderDlg::UpdateInfo()
{
  // Only a convex mesh has a single surface. A triangle mesh gets one per material slot of the
  // model, filled in by the asset transform.
  const bool bHasSurface = GetSelectedKind() == ezMeshColliderKind::ConvexHull;
  labelSurface->setVisible(bHasSurface);
  SurfaceAsset->setVisible(bHasSurface);
  SurfaceButton->setVisible(bHasSurface);
  SurfaceClearButton->setVisible(bHasSurface);

  ezStringBuilder sWarning;

  if (m_pSource == nullptr)
  {
    // nothing to validate: the paths are decided per mesh, and every problem with one mesh is a skip
  }
  else if (m_pSource->m_bIsPrimitive)
  {
    sWarning = "This mesh asset uses a procedural primitive, not a model file, so no collision mesh can be generated from it.";
  }
  else if (m_pSource->m_sMeshFile.IsEmpty())
  {
    sWarning = "The source file of this mesh asset could not be read, so no collision mesh can be generated from it.";
  }
  else
  {
    const ezString sTyped = qtToEzString(ColliderPath->text());

    ezStringBuilder sAbsolute;
    if (sTyped.IsEmpty())
    {
      sWarning = "Enter a path for the collision mesh asset.";
    }
    else if (ezMeshColliderCreator::ResolveDisplayPath(sTyped, sAbsolute).Failed())
    {
      sWarning = "This path does not start with the name of a data directory.";
    }
    else if (ezOSFile::ExistsFile(sAbsolute) && !OverwriteExisting->isChecked())
    {
      sWarning = "This file already exists. Tick the box below to overwrite it, or choose a different name.";
    }
  }

  WarningLabel->setText(ezMakeQString(sWarning));

  if (QPushButton* pOk = ButtonBox->button(QDialogButtonBox::Ok))
  {
    pOk->setEnabled(sWarning.IsEmpty());
  }
}

void ezQtCreateMeshColliderDlg::on_ColliderType_currentIndexChanged(int index)
{
  UpdateSuggestedPath();
  UpdateInfo();
}

void ezQtCreateMeshColliderDlg::on_OverwriteExisting_toggled(bool checked)
{
  // the suggestion dodges an existing file by appending a number, which overwriting no longer wants
  UpdateSuggestedPath();
  UpdateInfo();
}

void ezQtCreateMeshColliderDlg::on_ColliderPath_textChanged(const QString& text)
{
  if (!m_bSettingPath)
  {
    m_bPathIsSuggestion = false;
  }

  UpdateInfo();
}

void ezQtCreateMeshColliderDlg::on_BrowseButton_clicked()
{
  const bool bConvex = GetSelectedKind() == ezMeshColliderKind::ConvexHull;

  const QString sFilter = bConvex ? QLatin1String("Convex Collision Mesh (*.ezJoltConvexCollisionMeshAsset)")
                                  : QLatin1String("Collision Mesh (*.ezJoltCollisionMeshAsset)");

  // the file dialog needs a real path, the line edit holds a data directory relative one
  ezStringBuilder sStart;
  if (ezMeshColliderCreator::ResolveDisplayPath(qtToEzString(ColliderPath->text()), sStart).Failed() && m_pSource != nullptr)
  {
    sStart = ezMeshColliderCreator::SuggestColliderPath(*m_pSource, GetSelectedKind());
  }

  QString sFile = QFileDialog::getSaveFileName(this, QLatin1String("Create Collision Mesh"), ezMakeQString(sStart), sFilter,
    nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  // a path the user browsed to must survive a change of the collider type
  m_bPathIsSuggestion = false;
  ColliderPath->setText(ezMakeQString(ezMeshColliderCreator::MakeDisplayPath(qtToEzString(sFile))));
}

void ezQtCreateMeshColliderDlg::on_SurfaceButton_clicked()
{
  const ezUuid current = ezConversionUtils::ConvertStringToUuid(m_sSurface);

  ezQtAssetBrowserDlg dlg(this, current, "CompatibleAsset_Surface");
  if (dlg.exec() == 0)
    return;

  const ezUuid selected = dlg.GetSelectedAssetGuid();
  if (!selected.IsValid())
    return;

  // The guid is what goes into the asset, the path is only shown.
  ezStringBuilder sGuid;
  ezConversionUtils::ToString(selected, sGuid);
  m_sSurface = sGuid;

  SurfaceAsset->setText(ezMakeQString(dlg.GetSelectedAssetPathRelative()));
}

void ezQtCreateMeshColliderDlg::on_SurfaceClearButton_clicked()
{
  m_sSurface.Clear();
  SurfaceAsset->clear();
}

void ezQtCreateMeshColliderDlg::on_ButtonBox_accepted()
{
  // Left empty for several meshes, which is what makes the creator use each mesh's default path.
  m_Options.m_sColliderPath = (m_pSource != nullptr) ? qtToEzString(ColliderPath->text()) : ezString();
  m_Options.m_Kind = GetSelectedKind();
  s_LastKind = m_Options.m_Kind;
  m_Options.m_sSurface = (GetSelectedKind() == ezMeshColliderKind::ConvexHull) ? m_sSurface : ezString();
  m_Options.m_bOverwriteExisting = OverwriteExisting->isChecked();

  // Not remembered for a large selection, where the box was unticked by us rather than by the user.
  if (m_uiMeshCount <= s_uiMaxAutoOpen)
  {
    s_bOpenAfterCreate = OpenAfterCreate->isChecked();
  }

  m_Options.m_bOpenAfterCreate = OpenAfterCreate->isChecked();

  accept();
}

void ezQtCreateMeshColliderDlg::on_ButtonBox_rejected()
{
  reject();
}
