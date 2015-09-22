#include <PCH.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

#include <QHBoxLayout>
#include <QToolButton>
#include <QLabel>
#include <Assets/AssetDocumentManager.h>
#include <QMenu>
#include <Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>

ezAssetLineEdit::ezAssetLineEdit(QWidget* parent /*= nullptr*/) : QLineEdit(parent)
{

}

void ezAssetLineEdit::dragMoveEvent(QDragMoveEvent *e)
{
  if (e->mimeData()->hasUrls())
  {
    e->acceptProposedAction();
  }
  else
  {
    QLineEdit::dragMoveEvent(e);
  }
}

void ezAssetLineEdit::dragEnterEvent(QDragEnterEvent * e)
{
  if (e->mimeData()->hasUrls())
  {
    e->acceptProposedAction();
  }
  else
  {
    QLineEdit::dragEnterEvent(e);
  }
}

void ezAssetLineEdit::dropEvent(QDropEvent* e)
{
  if (e->source() == this)
  {
    QLineEdit::dropEvent(e);
    return;
  }

  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();
    setText(str);
    return;
  }


  if (e->mimeData()->hasText())
  {
    QString str = e->mimeData()->text();
    setText(str);
    return;
  }
}


ezAssetBrowserPropertyWidget::ezAssetBrowserPropertyWidget() : ezStandardPropertyBaseWidget()
{
  m_uiThumbnailID = 0;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new ezAssetLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setFocusProxy(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pWidget, SIGNAL(textChanged(const QString&)), this, SLOT(on_TextChanged_triggered(const QString&))) != nullptr, "signal/slot connection failed");

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("..."));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
  m_pButton->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  EZ_VERIFY(connect(m_pButton, SIGNAL(clicked()), this, SLOT(on_BrowseFile_clicked())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pButton, &QWidget::customContextMenuRequested, this, &ezAssetBrowserPropertyWidget::on_customContextMenuRequested) != nullptr, "signal/slot connection failed");

  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);

  EZ_VERIFY(connect(QtImageCache::GetInstance(), &QtImageCache::ImageLoaded, this, &ezAssetBrowserPropertyWidget::ThumbnailLoaded) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(QtImageCache::GetInstance(), &QtImageCache::ImageInvalidated, this, &ezAssetBrowserPropertyWidget::ThumbnailInvalidated) != nullptr, "signal/slot connection failed");
}

void ezAssetBrowserPropertyWidget::OnInit()
{
  EZ_ASSERT_DEV(m_pProp->GetAttributeByType<ezAssetBrowserAttribute>() != nullptr, "ezPropertyEditorFileBrowserWidget was created without a ezAssetBrowserAttribute!");
}

void ezAssetBrowserPropertyWidget::UpdateThumbnail(const ezUuid& guid, const char* szThumbnailPath)
{
  const QPixmap* pThumbnailPixmap = nullptr;

  if (guid.IsValid())
  {
    ezUInt64 uiUserData1, uiUserData2;
    m_AssetGuid.GetValues(uiUserData1, uiUserData2);

    pThumbnailPixmap = QtImageCache::QueryPixmap(szThumbnailPath, QModelIndex(), QVariant(uiUserData1), QVariant(uiUserData2), &m_uiThumbnailID);
  }

  if (pThumbnailPixmap)
  {
    m_pButton->setIcon(QIcon(pThumbnailPixmap->scaledToWidth(16, Qt::TransformationMode::SmoothTransformation)));
    m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
  }
  else
  {
    m_pButton->setIcon(QIcon());
    m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  }
}

void ezAssetBrowserPropertyWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);

  if (!value.IsValid())
  {
    m_pWidget->setPlaceholderText(QStringLiteral("<Multiple Values>"));
  }
  else
  {
    ezStringBuilder sText = value.ConvertTo<ezString>();
    m_AssetGuid = ezUuid();
    ezStringBuilder sThumbnailPath;

    if (ezConversionUtils::IsStringUuid(sText))
    {
      m_AssetGuid = ezConversionUtils::ConvertStringToUuid(sText);

      const auto* pAsset = ezAssetCurator::GetInstance()->GetAssetInfo(m_AssetGuid);

      if (pAsset)
      {
        sText = pAsset->m_sRelativePath;

        sThumbnailPath = ezAssetDocumentManager::GenerateResourceThumbnailPath(pAsset->m_sAbsolutePath);
      }
      else
        m_AssetGuid = ezUuid();
    }

    UpdateThumbnail(m_AssetGuid, sThumbnailPath);

    {
      auto pal = m_pWidget->palette();
      pal.setColor(QPalette::Text, m_AssetGuid.IsValid() ? QColor::fromRgb(182, 255, 0) : QColor::fromRgb(255, 170, 0));
      m_pWidget->setPalette(pal);

      if (m_AssetGuid.IsValid())
        m_pWidget->setToolTip(QStringLiteral("The selected file resolved to a valid asset GUID"));
      else
        m_pWidget->setToolTip(QStringLiteral("The selected file is not a valid asset"));
    }

    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(sText.GetData()));

  }
}

void ezAssetBrowserPropertyWidget::on_TextFinished_triggered()
{
  ezStringBuilder sText = m_pWidget->text().toUtf8().data();

  const auto* pAsset = ezAssetCurator::GetInstance()->FindAssetInfo(sText);

  if (pAsset)
  {
    sText = ezConversionUtils::ToString(pAsset->m_Info.m_DocumentID);
  }

  BroadcastValueChanged(sText.GetData());
}


void ezAssetBrowserPropertyWidget::on_TextChanged_triggered(const QString& value)
{
  if (!hasFocus())
    on_TextFinished_triggered();
}

void ezAssetBrowserPropertyWidget::ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2)
{
  const ezUuid guid(UserData1.toULongLong(), UserData2.toULongLong());

  if (guid == m_AssetGuid)
  {
    UpdateThumbnail(guid, sPath.toUtf8().data());
  }
}


void ezAssetBrowserPropertyWidget::ThumbnailInvalidated(QString sPath, ezUInt32 uiImageID)
{
  if (m_uiThumbnailID == uiImageID)
  {
    UpdateThumbnail(ezUuid(), "");
  }
}


void ezAssetBrowserPropertyWidget::on_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;

  const bool bAsset =m_AssetGuid.IsValid();

  m.setDefaultAction(m.addAction(QIcon(), QLatin1String("Select Asset"), this, SLOT(on_BrowseFile_clicked())));
  m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document16.png")), QLatin1String("Open Asset"), this, SLOT(OnOpenAssetDocument()))->setEnabled(bAsset);
  m.addAction(QIcon(), QLatin1String("Select in Asset Browser"), this, SLOT(OnSelectInAssetBrowser()))->setEnabled(bAsset);
  m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder16.png")), QLatin1String("Open Containing Folder"), this, SLOT(OnOpenExplorer()));

  m.exec(m_pButton->mapToGlobal(pt));
}

void ezAssetBrowserPropertyWidget::OnOpenAssetDocument()
{
  ezEditorApp::GetInstance()->OpenDocument(ezAssetCurator::GetInstance()->GetAssetInfo(m_AssetGuid)->m_sAbsolutePath);
}

void ezAssetBrowserPropertyWidget::OnSelectInAssetBrowser()
{
  ezAssetBrowserPanel::GetInstance()->AssetBrowserWidget->SetSelectedAsset(ezAssetCurator::GetInstance()->GetAssetInfo(m_AssetGuid)->m_sRelativePath);
}

void ezAssetBrowserPropertyWidget::OnOpenExplorer()
{
  ezString sPath;

  if (m_AssetGuid.IsValid())
  {
    sPath = ezAssetCurator::GetInstance()->GetAssetInfo(m_AssetGuid)->m_sAbsolutePath;
  }
  else
  {
    sPath = m_pWidget->text().toUtf8().data();
    if (!ezEditorApp::GetInstance()->MakeDataDirectoryRelativePathAbsolute(sPath))
      return;
  }

  ezUIServices::OpenInExplorer(sPath);
}

void ezAssetBrowserPropertyWidget::on_BrowseFile_clicked()
{
  ezString sFile = m_pWidget->text().toUtf8().data();
  const ezAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<ezAssetBrowserAttribute>();

  ezAssetBrowserDlg dlg(this, sFile, pAssetAttribute->GetTypeFilter());
  if (dlg.exec() == 0)
    return;

  sFile = dlg.GetSelectedAssetGuid();

  if (sFile.IsEmpty())
  {
    sFile = dlg.GetSelectedAssetPathRelative();

    if (sFile.IsEmpty())
    {
      sFile = dlg.GetSelectedAssetPathAbsolute();

      ezEditorApp::GetInstance()->MakePathDataDirectoryRelative(sFile);
    }
  }

  if (sFile.IsEmpty())
    return;

  m_pWidget->setText(sFile.GetData());
  on_TextFinished_triggered();
}

