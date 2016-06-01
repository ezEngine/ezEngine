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
#include "ToolsFoundation/Assets/AssetFileExtensionWhitelist.h"
#include <QClipboard>


ezQtAssetPropertyWidget::ezQtAssetPropertyWidget() : ezQtStandardPropertyWidget()
{
  m_uiThumbnailID = 0;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new ezQtAssetLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  m_pWidget->m_pOwner = this;
  setFocusProxy(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pWidget, SIGNAL(textChanged(const QString&)), this, SLOT(on_TextChanged_triggered(const QString&))) != nullptr, "signal/slot connection failed");

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("..."));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
  m_pButton->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  EZ_VERIFY(connect(m_pButton, SIGNAL(clicked()), this, SLOT(on_BrowseFile_clicked())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pButton, &QWidget::customContextMenuRequested, this, &ezQtAssetPropertyWidget::on_customContextMenuRequested) != nullptr, "signal/slot connection failed");

  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);

  EZ_VERIFY(connect(ezQtImageCache::GetSingleton(), &ezQtImageCache::ImageLoaded, this, &ezQtAssetPropertyWidget::ThumbnailLoaded) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(ezQtImageCache::GetSingleton(), &ezQtImageCache::ImageInvalidated, this, &ezQtAssetPropertyWidget::ThumbnailInvalidated) != nullptr, "signal/slot connection failed");
}


bool ezQtAssetPropertyWidget::IsValidAssetType(const char* szAssetReference) const
{
  const ezAssetInfo* pAsset = nullptr;

  if (!ezConversionUtils::IsStringUuid(szAssetReference))
  {
    pAsset = ezAssetCurator::GetSingleton()->FindAssetInfo(szAssetReference);

    if (!pAsset)
    {
      const ezAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<ezAssetBrowserAttribute>();

      // if this file type is on the asset whitelist for this asset type, let it through
      return ezAssetFileExtensionWhitelist::IsFileOnAssetWhitelist(pAssetAttribute->GetTypeFilter(), szAssetReference);
    }
  }
  else
  {
    const ezUuid AssetGuid = ezConversionUtils::ConvertStringToUuid(szAssetReference);

    pAsset = ezAssetCurator::GetSingleton()->GetAssetInfo(AssetGuid);
  }

  // invalid asset in general
  if (!pAsset)
    return false;

  const ezAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<ezAssetBrowserAttribute>();

  const ezStringBuilder sTypeFilter(";", pAsset->m_Info.m_sAssetTypeName, ";");
  return ezStringUtils::FindSubString_NoCase(pAssetAttribute->GetTypeFilter(), sTypeFilter) != nullptr;
}

void ezQtAssetPropertyWidget::OnInit()
{
  EZ_ASSERT_DEV(m_pProp->GetAttributeByType<ezAssetBrowserAttribute>() != nullptr, "ezQtAssetPropertyWidget was created without a ezAssetBrowserAttribute!");
}

void ezQtAssetPropertyWidget::UpdateThumbnail(const ezUuid& guid, const char* szThumbnailPath)
{
  const QPixmap* pThumbnailPixmap = nullptr;

  if (guid.IsValid())
  {
    ezUInt64 uiUserData1, uiUserData2;
    m_AssetGuid.GetValues(uiUserData1, uiUserData2);

    pThumbnailPixmap = ezQtImageCache::QueryPixmap(szThumbnailPath, QModelIndex(), QVariant(uiUserData1), QVariant(uiUserData2), &m_uiThumbnailID);
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

void ezQtAssetPropertyWidget::InternalSetValue(const ezVariant& value)
{
  QtScopedBlockSignals b(m_pWidget);
  QtScopedBlockSignals b2(m_pButton);

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
      if (!IsValidAssetType(sText))
      {
        m_uiThumbnailID = 0;

        m_pWidget->setText(QString());
        m_pWidget->setPlaceholderText(QStringLiteral("<Selected Invalid Asset Type>"));

        m_pButton->setIcon(QIcon());
        m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);

        auto pal = m_pWidget->palette();
        pal.setColor(QPalette::Text, Qt::red);
        m_pWidget->setPalette(pal);

        return;
      }

      m_AssetGuid = ezConversionUtils::ConvertStringToUuid(sText);

      const auto* pAsset = ezAssetCurator::GetSingleton()->GetAssetInfo(m_AssetGuid);

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

void ezQtAssetPropertyWidget::on_TextFinished_triggered()
{
  ezStringBuilder sText = m_pWidget->text().toUtf8().data();

  const auto* pAsset = ezAssetCurator::GetSingleton()->FindAssetInfo(sText);

  if (pAsset)
  {
    sText = ezConversionUtils::ToString(pAsset->m_Info.m_DocumentID);
  }

  BroadcastValueChanged(sText.GetData());
}


void ezQtAssetPropertyWidget::on_TextChanged_triggered(const QString& value)
{
  if (!hasFocus())
    on_TextFinished_triggered();
}

void ezQtAssetPropertyWidget::ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2)
{
  const ezUuid guid(UserData1.toULongLong(), UserData2.toULongLong());

  if (guid == m_AssetGuid)
  {
    UpdateThumbnail(guid, sPath.toUtf8().data());
  }
}


void ezQtAssetPropertyWidget::ThumbnailInvalidated(QString sPath, ezUInt32 uiImageID)
{
  if (m_uiThumbnailID == uiImageID)
  {
    UpdateThumbnail(ezUuid(), "");
  }
}


void ezQtAssetPropertyWidget::on_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;

  const bool bAsset =m_AssetGuid.IsValid();

  m.setDefaultAction(m.addAction(QIcon(), QLatin1String("Select Asset"), this, SLOT(on_BrowseFile_clicked())));
  m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document16.png")), QLatin1String("Open Asset"), this, SLOT(OnOpenAssetDocument()))->setEnabled(bAsset);
  m.addAction(QIcon(), QLatin1String("Select in Asset Browser"), this, SLOT(OnSelectInAssetBrowser()))->setEnabled(bAsset);
  m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder16.png")), QLatin1String("Open Containing Folder"), this, SLOT(OnOpenExplorer()));
  m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/DocumentGuid16.png")), QLatin1String("Copy Asset Guid"), this, SLOT(OnCopyAssetGuid()));

  m.exec(m_pButton->mapToGlobal(pt));
}

void ezQtAssetPropertyWidget::OnOpenAssetDocument()
{
  ezQtEditorApp::GetSingleton()->OpenDocument(ezAssetCurator::GetSingleton()->GetAssetInfo(m_AssetGuid)->m_sAbsolutePath);
}

void ezQtAssetPropertyWidget::OnSelectInAssetBrowser()
{
  ezQtAssetBrowserPanel::GetSingleton()->AssetBrowserWidget->SetSelectedAsset(ezAssetCurator::GetSingleton()->GetAssetInfo(m_AssetGuid)->m_sRelativePath);
}

void ezQtAssetPropertyWidget::OnOpenExplorer()
{
  ezString sPath;

  if (m_AssetGuid.IsValid())
  {
    sPath = ezAssetCurator::GetSingleton()->GetAssetInfo(m_AssetGuid)->m_sAbsolutePath;
  }
  else
  {
    sPath = m_pWidget->text().toUtf8().data();
    if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
      return;
  }

  ezUIServices::OpenInExplorer(sPath);
}


void ezQtAssetPropertyWidget::OnCopyAssetGuid()
{
  ezString sGuid;

  if (m_AssetGuid.IsValid())
  {
    sGuid = ezConversionUtils::ToString(m_AssetGuid);
  }
  else
  {
    sGuid = m_pWidget->text().toUtf8().data();
    if (!ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sGuid))
      return;
  }

  QClipboard* clipboard = QApplication::clipboard();
  QMimeData* mimeData = new QMimeData();
  mimeData->setText(QString::fromUtf8(sGuid.GetData()));
  clipboard->setMimeData(mimeData);

}

void ezQtAssetPropertyWidget::on_BrowseFile_clicked()
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

      ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sFile);
    }
  }

  if (sFile.IsEmpty())
    return;

  InternalSetValue(sFile);

  on_TextFinished_triggered();
}

