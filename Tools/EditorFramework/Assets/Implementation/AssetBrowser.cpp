#include <PCH.h>
#include <EditorFramework/Assets/AssetBrowser.moc.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Basics.h>
#include <QVBoxLayout>
#include <QScrollBar>

ezAssetBrowser::ezAssetBrowser(QWidget* parent) : QWidget(parent)
{
  setupUi(this);

  m_pModel = new ezAssetCuratorModel(this);

  ListAssets->setModel(m_pModel);
  ListAssets->SetIconScale(IconSizeSlider->value());
  on_ButtonIconMode_clicked();

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  //connect(ListAssets, SIGNAL(ViewZoomed(ezInt32 iDelta)), this, SLOT());
#if (QT_VERSION < QT_VERSION_CHECK(5, 3, 0))
  // Qt 5.2's scroll speed in list views is kinda broken
  // https://bugreports.qt.io/browse/QTBUG-34378

  /// \todo Remove this once we are on Qt Version 5.4 or so

  ListAssets->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
  ListAssets->verticalScrollBar()->setSingleStep(64);
#endif

  // Tool Bar
  {
    ezToolBarActionMapView* pToolBar = new ezToolBarActionMapView(this);
    ezActionContext context;
    context.m_sMapping = "AssetBrowserToolBar";
    context.m_pDocument = nullptr;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("TextureAssetWindowToolBar");
    ToolBarLayout->insertWidget(0, pToolBar);
  }

  EZ_VERIFY(connect(m_pModel, SIGNAL(TextFilterChanged()), this, SLOT(OnTextFilterChanged())) != nullptr, "signal/slot connection failed");

  ezSet<ezString> KnownAssetTypes;

  for (auto docman : ezDocumentManagerBase::GetAllDocumentManagers())
  {
    if (!docman->GetDynamicRTTI()->IsDerivedFrom<ezAssetDocumentManager>())
      continue;

    const ezAssetDocumentManager* pAssetDocMan = static_cast<const ezAssetDocumentManager*>(docman);

    pAssetDocMan->QuerySupportedAssetTypes(KnownAssetTypes);
  }

  {
    QtScopedBlockSignals block(ListTypeFilter);

    ezStringBuilder sIconName;

    for (const auto& key : KnownAssetTypes)
    {
      sIconName.Set(":/AssetIcons/", key);
      sIconName.ReplaceAll(" ", "_");

      QListWidgetItem* pItem = new QListWidgetItem(QIcon(QString::fromUtf8(sIconName.GetData())), QString::fromUtf8(key.GetData()));
      pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);
      pItem->setCheckState(Qt::CheckState::Unchecked);

      ListTypeFilter->addItem(pItem);
    }
  }
}

ezAssetBrowser::~ezAssetBrowser()
{
  ListAssets->setModel(nullptr);
}

void ezAssetBrowser::on_ListAssets_doubleClicked(const QModelIndex& index)
{
  ezEditorApp::GetInstance()->OpenDocument(m_pModel->data(index, Qt::UserRole + 1).toString().toUtf8().data());

}

void ezAssetBrowser::on_ButtonListMode_clicked()
{
  m_pModel->SetIconMode(false);
  ListAssets->SetIconMode(false);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  if (!selection.isEmpty())
    ListAssets->scrollTo(selection[0]);

  ButtonListMode->setChecked(true);
  ButtonIconMode->setChecked(false);
}

void ezAssetBrowser::on_ButtonIconMode_clicked()
{
  m_pModel->SetIconMode(true);
  ListAssets->SetIconMode(true);

  QModelIndexList selection = ListAssets->selectionModel()->selectedIndexes();

  if (!selection.isEmpty())
    ListAssets->scrollTo(selection[0]);

  ButtonListMode->setChecked(false);
  ButtonIconMode->setChecked(true);
}

void ezAssetBrowser::on_IconSizeSlider_valueChanged(int iValue)
{
  ListAssets->SetIconScale(iValue);
}

void ezAssetBrowser::on_ListAssets_ViewZoomed(ezInt32 iIconSizePercentage)
{
  QtScopedBlockSignals block(IconSizeSlider);
  IconSizeSlider->setValue(iIconSizePercentage);
}

void ezAssetBrowser::OnTextFilterChanged()
{
  LineSearchFilter->setText(QString::fromUtf8(m_pModel->GetTextFilter()));
}

void ezAssetBrowser::on_LineSearchFilter_textEdited(const QString& text)
{
  m_pModel->SetTextFilter(text.toUtf8().data());
}

void ezAssetBrowser::on_ButtonClearSearch_clicked()
{
  m_pModel->SetTextFilter("");
}

