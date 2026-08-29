#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Dialogs/CreateMeshPrefabDlg.moc.h>
#include <Foundation/IO/OSFile.h>
#include <QFileDialog>
#include <QPushButton>
#include <ToolsFoundation/Project/ToolsProject.h>

ezInt32 ezQtCreateMeshPrefabDlg::s_iPhysicsMode = ezMeshPrefabPhysics::None;
ezInt32 ezQtCreateMeshPrefabDlg::s_iCollisionLayer = 0;
bool ezQtCreateMeshPrefabDlg::s_bOpenAfterCreate = true;

ezQtCreateMeshPrefabDlg::ezQtCreateMeshPrefabDlg(const ezMeshPrefabSource& source, QWidget* pParent)
  : ezQtDialog(pParent)
  , m_pSource(&source)
{
  Setup();

  UpdateSuggestedPath();

  {
    // The animated components need a skeleton, which only an animated mesh asset has. The other way
    // round is fine: an animated mesh can be rendered by a static component.
    RenderComponent->addItem("ezMeshComponent", QVariant("ezMeshComponent"));
    RenderComponent->addItem("ezLodMeshComponent", QVariant("ezLodMeshComponent"));

    if (source.m_bAnimated)
    {
      RenderComponent->addItem("ezAnimatedMeshComponent", QVariant("ezAnimatedMeshComponent"));
      RenderComponent->addItem("ezLodAnimatedMeshComponent", QVariant("ezLodAnimatedMeshComponent"));
    }

    const ezStringView sDefault = source.GetDefaultRenderComponentType();
    RenderComponent->setCurrentIndex(RenderComponent->findData(QVariant(ezMakeQString(sDefault))));
  }

  UpdateLodInfo();

  if (PhysicsGroup->isVisible())
  {
    // the box modes need bounds, which only exist once the mesh has been transformed
    if (!source.m_bHasBounds)
    {
      const char* szReason = "The bounds of this mesh are unknown. Transform the mesh asset to make this available.";

      for (ezInt32 i : {(ezInt32)ezMeshPrefabPhysics::StaticBox, (ezInt32)ezMeshPrefabPhysics::DynamicBox})
      {
        const int idx = PhysicsMode->findData(QVariant(i));
        PhysicsMode->setItemData(idx, QVariant(0), Qt::UserRole - 1); // disables the item
        PhysicsMode->setItemData(idx, QString(szReason), Qt::ToolTipRole);
      }
    }

    // generating a collision mesh needs the original model file
    if (source.m_sMeshFile.IsEmpty())
    {
      const char* szReason = "The source file of this mesh asset could not be read, so no collision mesh can be generated from it.";

      for (ezInt32 i : {(ezInt32)ezMeshPrefabPhysics::StaticTriangleMesh, (ezInt32)ezMeshPrefabPhysics::StaticConvexHull, (ezInt32)ezMeshPrefabPhysics::DynamicConvexHull})
      {
        const int idx = PhysicsMode->findData(QVariant(i));
        PhysicsMode->setItemData(idx, QVariant(0), Qt::UserRole - 1);
        PhysicsMode->setItemData(idx, QString(szReason), Qt::ToolTipRole);
      }
    }

    // what was decided for this mesh beats what was picked for some other mesh last time
    const ezEnum<ezMeshPrefabPhysics> detected = source.GetDefaultPhysics();
    const ezInt32 iPreferred = (detected != ezMeshPrefabPhysics::None) ? (ezInt32)detected.GetValue() : s_iPhysicsMode;

    const int idxPreferred = PhysicsMode->findData(QVariant(iPreferred));
    if (idxPreferred >= 0 && (PhysicsMode->itemData(idxPreferred, Qt::UserRole - 1).isNull() || PhysicsMode->itemData(idxPreferred, Qt::UserRole - 1).toBool()))
    {
      PhysicsMode->setCurrentIndex(idxPreferred);
    }

    if (detected != ezMeshPrefabPhysics::None)
    {
      PhysicsMode->setToolTip("A collision mesh built from this model already exists and will be reused.");
    }
  }

  UpdateWarnings();
}

ezQtCreateMeshPrefabDlg::ezQtCreateMeshPrefabDlg(ezUInt32 uiMeshCount, QWidget* pParent)
  : ezQtDialog(pParent)
  , m_uiMeshCount(uiMeshCount)
{
  Setup();

  // with several meshes there is no one path to pick, each prefab goes next to its own mesh
  PrefabPath->setEnabled(false);
  BrowseButton->setEnabled(false);
  m_bSettingPath = true;
  PrefabPath->setText(QLatin1String("<next to each mesh asset>"));
  m_bSettingPath = false;

  // which component fits depends on the mesh - animated or not, with LODs or without
  RenderComponent->addItem("<the one that fits each mesh>");
  RenderComponent->setEnabled(false);

  LodInfo->setText("LOD meshes found next to a mesh are added to its prefab, if its component uses them.");

  OverwriteExisting->setText(QLatin1String("Overwrite prefabs that already exist"));

  if (PhysicsGroup->isVisible())
  {
    const int idx = PhysicsMode->findData(QVariant(s_iPhysicsMode));
    if (idx >= 0)
    {
      PhysicsMode->setCurrentIndex(idx);
    }

    PhysicsMode->setToolTip("Meshes this cannot be applied to are skipped.");
  }

  // the box stays available, it is only unticked by default for a large selection
  OpenAfterCreate->setText(QString("Open all %1 prefabs after creating them").arg(uiMeshCount));
  OpenAfterCreate->setChecked(s_bOpenAfterCreate && uiMeshCount <= s_uiMaxAutoOpen);

  UpdateWarnings();
}

void ezQtCreateMeshPrefabDlg::Setup()
{
  setupUi(this);

  if (!ezMeshPrefabCreator::IsPhysicsAvailable())
  {
    PhysicsGroup->setVisible(false);
  }
  else
  {
    PhysicsMode->addItem("None", QVariant(ezMeshPrefabPhysics::None));
    PhysicsMode->addItem("Static - triangle collision mesh", QVariant(ezMeshPrefabPhysics::StaticTriangleMesh));
    PhysicsMode->addItem("Static - convex hull collision mesh", QVariant(ezMeshPrefabPhysics::StaticConvexHull));
    PhysicsMode->addItem("Static - box from mesh bounds", QVariant(ezMeshPrefabPhysics::StaticBox));
    PhysicsMode->addItem("Dynamic - convex hull collision mesh", QVariant(ezMeshPrefabPhysics::DynamicConvexHull));
    PhysicsMode->addItem("Dynamic - box from mesh bounds", QVariant(ezMeshPrefabPhysics::DynamicBox));

    CollisionLayer->setValue(s_iCollisionLayer);
  }

  OpenAfterCreate->setChecked(s_bOpenAfterCreate);
}

void ezQtCreateMeshPrefabDlg::UpdateLodInfo()
{
  if (m_pSource == nullptr)
    return;

  // only the LOD components use the extra meshes
  const ezString sRenderType = qtToEzString(RenderComponent->currentData().toString());
  const bool bLodComponent = (sRenderType == "ezLodMeshComponent") || (sRenderType == "ezLodAnimatedMeshComponent");

  if (m_pSource->m_LodGuids.IsEmpty())
  {
    LodInfo->setText("No LOD meshes were found next to this mesh.");
  }
  else if (bLodComponent)
  {
    LodInfo->setText(QString("Found %1 LOD mesh(es), they will be added to the component in order.").arg(m_pSource->m_LodGuids.GetCount()));
  }
  else
  {
    LodInfo->setText(QString("Found %1 LOD mesh(es), but this component does not use them.").arg(m_pSource->m_LodGuids.GetCount()));
  }
}

void ezQtCreateMeshPrefabDlg::UpdateSuggestedPath()
{
  if (m_pSource == nullptr || !m_bPathIsSuggestion)
    return;

  const ezString sPath = ezMeshPrefabCreator::MakeDisplayPath(
    ezMeshPrefabCreator::SuggestPrefabPath(*m_pSource, OverwriteExisting->isChecked()));

  // the change is our own, so the text handler must not treat it as the user typing a path
  m_bSettingPath = true;
  PrefabPath->setText(ezMakeQString(sPath));
  m_bSettingPath = false;
}

void ezQtCreateMeshPrefabDlg::UpdateWarnings()
{
  ezStringBuilder sWarning;

  if (m_pSource != nullptr)
  {
    const ezString sTyped = qtToEzString(PrefabPath->text());

    ezStringBuilder sAbsolute;
    if (sTyped.IsEmpty())
    {
      sWarning = "Enter a path for the prefab.";
    }
    else if (ezMeshPrefabCreator::ResolveDisplayPath(sTyped, sAbsolute).Failed())
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

void ezQtCreateMeshPrefabDlg::on_PrefabPath_textChanged(const QString& text)
{
  if (!m_bSettingPath)
  {
    m_bPathIsSuggestion = false;
  }

  UpdateWarnings();
}

void ezQtCreateMeshPrefabDlg::on_RenderComponent_currentIndexChanged(int index)
{
  UpdateLodInfo();
}

void ezQtCreateMeshPrefabDlg::on_OverwriteExisting_toggled(bool checked)
{
  // the suggestion dodges an existing file by appending a number, which overwriting no longer wants
  UpdateSuggestedPath();
  UpdateWarnings();
}

void ezQtCreateMeshPrefabDlg::on_BrowseButton_clicked()
{
  // the file dialog needs a real path, the line edit holds a data directory relative one
  ezStringBuilder sStart;
  if (ezMeshPrefabCreator::ResolveDisplayPath(qtToEzString(PrefabPath->text()), sStart).Failed() && m_pSource != nullptr)
  {
    sStart = ezMeshPrefabCreator::SuggestPrefabPath(*m_pSource);
  }

  QString sFile = QFileDialog::getSaveFileName(this, QLatin1String("Create Prefab"), ezMakeQString(sStart), QLatin1String("Prefab (*.ezPrefab)"),
    nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  // a path the user browsed to must survive a change of the overwrite box
  m_bPathIsSuggestion = false;
  PrefabPath->setText(ezMakeQString(ezMeshPrefabCreator::MakeDisplayPath(qtToEzString(sFile))));
}

void ezQtCreateMeshPrefabDlg::on_ButtonBox_accepted()
{
  // Both are left empty for several meshes, which is what makes the creator decide them per mesh.
  if (m_pSource != nullptr)
  {
    m_Options.m_sPrefabPath = qtToEzString(PrefabPath->text());
    m_Options.m_sRenderComponentType = qtToEzString(RenderComponent->currentData().toString());
  }

  m_Options.m_bOverwriteExisting = OverwriteExisting->isChecked();

  if (PhysicsGroup->isVisible())
  {
    s_iPhysicsMode = PhysicsMode->currentData().toInt();
    s_iCollisionLayer = CollisionLayer->value();

    m_Options.m_Physics = (ezMeshPrefabPhysics::Enum)s_iPhysicsMode;
    m_Options.m_uiCollisionLayer = (ezUInt8)s_iCollisionLayer;
  }

  // Not remembered for a large selection, where the box was unticked by us rather than by the user.
  if (m_uiMeshCount <= s_uiMaxAutoOpen)
  {
    s_bOpenAfterCreate = OpenAfterCreate->isChecked();
  }

  m_Options.m_bOpenAfterCreate = OpenAfterCreate->isChecked();

  accept();
}

void ezQtCreateMeshPrefabDlg::on_ButtonBox_rejected()
{
  reject();
}
