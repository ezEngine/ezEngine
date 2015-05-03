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
  EZ_VERIFY(connect(m_pModel, SIGNAL(TypeFilterChanged()), this, SLOT(OnTypeFilterChanged())) != nullptr, "signal/slot connection failed");

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

    // 'All' Filter
    {
      QListWidgetItem* pItem = new QListWidgetItem(QIcon(QLatin1String(":/AssetIcons/All")), QLatin1String("<All>"));
      pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);
      pItem->setCheckState(Qt::CheckState::Checked);

      ListTypeFilter->addItem(pItem);
    }

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

void ezAssetBrowser::OnTypeFilterChanged()
{
  ezStringBuilder sTemp;
  const ezStringBuilder sFilter = m_pModel->GetTypeFilter();

  bool bAnyChecked = false;

  for (ezInt32 i = 1; i < ListTypeFilter->count(); ++i)
  {
    sTemp.Set(";", ListTypeFilter->item(i)->text().toUtf8().data(), ";");

    bool bChecked = sFilter.FindSubString(sTemp) != nullptr;

    if (bChecked)
      bAnyChecked = true;
  }

  ListTypeFilter->item(0)->setCheckState(bAnyChecked ? Qt::Unchecked : Qt::Checked);
}

void ezAssetBrowser::on_LineSearchFilter_textEdited(const QString& text)
{
  m_pModel->SetTextFilter(text.toUtf8().data());
}

void ezAssetBrowser::on_ButtonClearSearch_clicked()
{
  m_pModel->SetTextFilter("");
}

void ezAssetBrowser::on_ListTypeFilter_itemChanged(QListWidgetItem* item)
{
  QtScopedBlockSignals block(ListTypeFilter);

  if (item->text() == "<All>")
  {
    if (item->checkState() == Qt::Checked)
    {
      // deactivate all others
      for (ezInt32 i = 1; i < ListTypeFilter->count(); ++i)
      {
        ListTypeFilter->item(i)->setCheckState(Qt::Unchecked);
      }
    }
  }
  else
  {
    if (item->checkState() == Qt::Checked)
    {
      // deactivate the 'all' button
      ListTypeFilter->item(0)->setCheckState(Qt::Unchecked);
    }
    else
    {
      bool bAnyChecked = false;

      for (ezInt32 i = 1; i < ListTypeFilter->count(); ++i)
      {
        if (ListTypeFilter->item(i)->checkState() == Qt::Checked)
          bAnyChecked = true;
      }

      // activate the 'All' item
      if (!bAnyChecked)
        ListTypeFilter->item(0)->setCheckState(Qt::Checked);
    }
  }

  ezStringBuilder sFilter;

  for (ezInt32 i = 1; i < ListTypeFilter->count(); ++i)
  {
    if (ListTypeFilter->item(i)->checkState() == Qt::Checked)
      sFilter.Append(";", ListTypeFilter->item(i)->text().toUtf8().data(), ";");
  }

  m_pModel->SetTypeFilter(sFilter);
}