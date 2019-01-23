#include <PCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <QClipboard>
#include <QFileDialog>
#include <QMenu>
#include <QMimeData>
#include <QToolButton>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

ezQtAssetPropertyWidget::ezQtAssetPropertyWidget()
    : ezQtStandardPropertyWidget()
{
  m_uiThumbnailID = 0;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  m_pLayout->setSpacing(0);
  setLayout(m_pLayout);

  m_pWidget = new ezQtAssetLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  m_pWidget->m_pOwner = this;
  setFocusProxy(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered())) != nullptr,
            "signal/slot connection failed");
  EZ_VERIFY(connect(m_pWidget, SIGNAL(textChanged(const QString&)), this, SLOT(on_TextChanged_triggered(const QString&))) != nullptr,
            "signal/slot connection failed");

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("..."));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
  m_pButton->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  EZ_VERIFY(connect(m_pButton, SIGNAL(clicked()), this, SLOT(on_BrowseFile_clicked())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pButton, &QWidget::customContextMenuRequested, this, &ezQtAssetPropertyWidget::on_customContextMenuRequested) !=
                nullptr,
            "signal/slot connection failed");

  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);

  EZ_VERIFY(connect(ezQtImageCache::GetSingleton(), &ezQtImageCache::ImageLoaded, this, &ezQtAssetPropertyWidget::ThumbnailLoaded) !=
                nullptr,
            "signal/slot connection failed");
  EZ_VERIFY(connect(ezQtImageCache::GetSingleton(), &ezQtImageCache::ImageInvalidated, this,
                    &ezQtAssetPropertyWidget::ThumbnailInvalidated) != nullptr,
            "signal/slot connection failed");
}


bool ezQtAssetPropertyWidget::IsValidAssetType(const char* szAssetReference) const
{
  ezAssetCurator::ezLockedSubAsset pAsset;

  if (!ezConversionUtils::IsStringUuid(szAssetReference))
  {
    pAsset = ezAssetCurator::GetSingleton()->FindSubAsset(szAssetReference);

    if (pAsset == nullptr)
    {
      const ezAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<ezAssetBrowserAttribute>();

      // if this file type is on the asset whitelist for this asset type, let it through
      return ezAssetFileExtensionWhitelist::IsFileOnAssetWhitelist(pAssetAttribute->GetTypeFilter(), szAssetReference);
    }
  }
  else
  {
    const ezUuid AssetGuid = ezConversionUtils::ConvertStringToUuid(szAssetReference);

    pAsset = ezAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);
  }

  // invalid asset in general
  if (pAsset == nullptr)
    return false;

  const ezAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<ezAssetBrowserAttribute>();

  if (ezStringUtils::IsEqual(pAssetAttribute->GetTypeFilter(), ";;")) // empty type list -> allows everything
    return true;

  const ezStringBuilder sTypeFilter(";", pAsset->m_Data.m_sAssetTypeName.GetData(), ";");
  return ezStringUtils::FindSubString_NoCase(pAssetAttribute->GetTypeFilter(), sTypeFilter) != nullptr;
}

void ezQtAssetPropertyWidget::OnInit()
{
  EZ_ASSERT_DEV(m_pProp->GetAttributeByType<ezAssetBrowserAttribute>() != nullptr,
                "ezQtAssetPropertyWidget was created without a ezAssetBrowserAttribute!");
}

void ezQtAssetPropertyWidget::UpdateThumbnail(const ezUuid& guid, const char* szThumbnailPath)
{
  if (IsUndead())
    return;
  const QPixmap* pThumbnailPixmap = nullptr;

  if (guid.IsValid())
  {
    ezUInt64 uiUserData1, uiUserData2;
    m_AssetGuid.GetValues(uiUserData1, uiUserData2);

    const ezAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<ezAssetBrowserAttribute>();
    ezStringBuilder sTypeFilter = pAssetAttribute->GetTypeFilter();
    sTypeFilter.Trim(" ;");

    pThumbnailPixmap = ezQtImageCache::GetSingleton()->QueryPixmapForType(sTypeFilter, szThumbnailPath, QModelIndex(),
                                                                          QVariant(uiUserData1), QVariant(uiUserData2), &m_uiThumbnailID);
  }

  m_pButton->setCursor(Qt::WhatsThisCursor);

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
  ezQtScopedBlockSignals b(m_pWidget);
  ezQtScopedBlockSignals b2(m_pButton);

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

      auto pAsset = ezAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid);

      if (pAsset)
      {
        pAsset->GetSubAssetIdentifier(sText);

        sThumbnailPath = ezAssetDocumentManager::GenerateResourceThumbnailPath(pAsset->m_pAssetInfo->m_sAbsolutePath);
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

void ezQtAssetPropertyWidget::FillAssetMenu(QMenu& menu)
{
  if (!menu.isEmpty())
    menu.addSeparator();

  const bool bAsset = m_AssetGuid.IsValid();
  menu.setDefaultAction(menu.addAction(QIcon(), QLatin1String("Select Asset"), this, SLOT(on_BrowseFile_clicked())));
  menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document16.png")), QLatin1String("Open Asset"), this,
                 SLOT(OnOpenAssetDocument()))
      ->setEnabled(bAsset);
  menu.addAction(QIcon(), QLatin1String("Select in Asset Browser"), this, SLOT(OnSelectInAssetBrowser()))->setEnabled(bAsset);
  menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder16.png")), QLatin1String("Open in Explorer"), this,
                 SLOT(OnOpenExplorer()));
  menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/DocumentGuid16.png")), QLatin1String("Copy Asset Guid"), this,
                 SLOT(OnCopyAssetGuid()));
  menu.addAction(QIcon(), QLatin1String("Create New Asset"), this, SLOT(OnCreateNewAsset()));
  menu.addAction(QIcon(":/GuiFoundation/Icons/Delete16.png"), QLatin1String("Clear Asset Reference"), this, SLOT(OnClearReference()));
}

void ezQtAssetPropertyWidget::on_TextFinished_triggered()
{
  ezStringBuilder sText = m_pWidget->text().toUtf8().data();

  auto pAsset = ezAssetCurator::GetSingleton()->FindSubAsset(sText);

  if (pAsset)
  {
    ezConversionUtils::ToString(pAsset->m_Data.m_Guid, sText);
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
  FillAssetMenu(m);

  m.exec(m_pButton->mapToGlobal(pt));
}

void ezQtAssetPropertyWidget::OnOpenAssetDocument()
{
  ezQtEditorApp::GetSingleton()->OpenDocumentQueued(ezAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid)->m_pAssetInfo->m_sAbsolutePath,
                                              GetSelection()[0].m_pObject);
}

void ezQtAssetPropertyWidget::OnSelectInAssetBrowser()
{
  ezQtAssetBrowserPanel::GetSingleton()->AssetBrowserWidget->SetSelectedAsset(m_AssetGuid);
  ezQtAssetBrowserPanel::GetSingleton()->raise();
}

void ezQtAssetPropertyWidget::OnOpenExplorer()
{
  ezString sPath;

  if (m_AssetGuid.IsValid())
  {
    sPath = ezAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid)->m_pAssetInfo->m_sAbsolutePath;
  }
  else
  {
    sPath = m_pWidget->text().toUtf8().data();
    if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
      return;
  }

  ezQtUiServices::OpenInExplorer(sPath, true);
}

void ezQtAssetPropertyWidget::OnCopyAssetGuid()
{
  ezStringBuilder sGuid;

  if (m_AssetGuid.IsValid())
  {
    ezConversionUtils::ToString(m_AssetGuid, sGuid);
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

void ezQtAssetPropertyWidget::OnCreateNewAsset()
{
  ezString sPath;

  // try to pick a good path
  {
    if (m_AssetGuid.IsValid())
    {
      sPath = ezAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid)->m_pAssetInfo->m_sAbsolutePath;
    }
    else
    {
      sPath = m_pWidget->text().toUtf8().data();
      ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
    }
  }

  const ezAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<ezAssetBrowserAttribute>();
  ezStringBuilder sTypeFilter = pAssetAttribute->GetTypeFilter();

  ezAssetDocumentManager* pAssetManToUse = nullptr;
  {
    const ezHybridArray<ezDocumentManager*, 16>& managers = ezDocumentManager::GetAllDocumentManagers();

    ezSet<ezString> documentTypes;

    for (ezDocumentManager* pMan : managers)
    {
      if (!pMan->GetDynamicRTTI()->IsDerivedFrom<ezAssetDocumentManager>())
        continue;

      ezAssetDocumentManager* pAssetMan = static_cast<ezAssetDocumentManager*>(pMan);

      documentTypes.Clear();
      pAssetMan->QuerySupportedAssetTypes(documentTypes);

      for (const ezString& assetType : documentTypes)
      {
        if (!sTypeFilter.FindSubString(assetType))
          continue;

        pAssetManToUse = pAssetMan;
        goto found;
      }
    }
  }

  return;

found:

  ezDynamicArray<const ezDocumentTypeDescriptor*> documentTypes;
  pAssetManToUse->GetSupportedDocumentTypes(documentTypes);
  const ezString sAssetType = documentTypes[0]->m_sDocumentTypeName;
  const ezString sExtension = documentTypes[0]->m_sFileExtension;

  ezStringBuilder sOutput = sPath;
  {
    ezStringBuilder sTemp = sOutput.GetFileDirectory();
    ezStringBuilder title("Create ", sAssetType), sFilter;

    sFilter.Format("{0} (*.{1})", sAssetType, sExtension);

    QString sStartDir = sTemp.GetData();
    QString sSelectedFilter = sExtension.GetData();
    sOutput = QFileDialog::getSaveFileName(QApplication::activeWindow(), title.GetData(), sStartDir, sFilter.GetData(), &sSelectedFilter,
                                           QFileDialog::Option::DontResolveSymlinks)
                  .toUtf8()
                  .data();

    if (sOutput.IsEmpty())
      return;
  }

  ezDocument* pDoc;
  if (pAssetManToUse->CreateDocument(sAssetType, sOutput, pDoc, ezDocumentFlags::RequestWindow | ezDocumentFlags::AddToRecentFilesList).m_Result.Succeeded())
  {
    pDoc->EnsureVisible();

    InternalSetValue(sOutput.GetData());
    on_TextFinished_triggered();
  }
}


void ezQtAssetPropertyWidget::OnClearReference()
{
  InternalSetValue("");
  on_TextFinished_triggered();
}

void ezQtAssetPropertyWidget::on_BrowseFile_clicked()
{
  ezStringBuilder sFile = m_pWidget->text().toUtf8().data();
  const ezAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<ezAssetBrowserAttribute>();

  ezQtAssetBrowserDlg dlg(this, m_AssetGuid, pAssetAttribute->GetTypeFilter());
  if (dlg.exec() == 0)
    return;

  ezUuid assetGuid = dlg.GetSelectedAssetGuid();
  if (assetGuid.IsValid())
    ezConversionUtils::ToString(assetGuid, sFile);

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

  InternalSetValue(sFile.GetData());

  on_TextFinished_triggered();
}
