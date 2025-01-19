#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezQtAssetPropertyWidget::ezQtAssetPropertyWidget()
  : ezQtStandardPropertyWidget()
{
  m_uiThumbnailID = 0;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  m_pLayout->setSpacing(0);
  setLayout(m_pLayout);

  m_pWidget = new ezQtAssetLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  m_pWidget->m_pOwner = this;
  setFocusProxy(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pWidget, SIGNAL(textChanged(const QString&)), this, SLOT(on_TextChanged_triggered(const QString&))) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pWidget, SIGNAL(OpenAsset()), this, SLOT(OnOpenAssetDocument())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pWidget, SIGNAL(SelectAsset()), this, SLOT(on_BrowseFile_clicked())) != nullptr, "signal/slot connection failed");

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("... "));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  m_pButton->setPopupMode(QToolButton::InstantPopup);

  QMenu* pMenu = new QMenu();
  pMenu->setToolTipsVisible(true);
  m_pButton->setMenu(pMenu);

  connect(pMenu, &QMenu::aboutToShow, this, &ezQtAssetPropertyWidget::OnShowMenu);

  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);

  EZ_VERIFY(connect(ezQtImageCache::GetSingleton(), &ezQtImageCache::ImageLoaded, this, &ezQtAssetPropertyWidget::ThumbnailLoaded) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(
    connect(ezQtImageCache::GetSingleton(), &ezQtImageCache::ImageInvalidated, this, &ezQtAssetPropertyWidget::ThumbnailInvalidated) != nullptr, "signal/slot connection failed");
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

  ezStringBuilder sTypeFilter(";", pAsset->m_Data.m_sSubAssetsDocumentTypeName, ";");

  if (ezStringUtils::FindSubString_NoCase(pAssetAttribute->GetTypeFilter(), sTypeFilter) != nullptr)
    return true;

  if (const ezDocumentTypeDescriptor* pDesc = ezDocumentManager::GetDescriptorForDocumentType(pAsset->m_Data.m_sSubAssetsDocumentTypeName))
  {
    for (const ezString& comp : pDesc->m_CompatibleTypes)
    {
      sTypeFilter.Set(";", comp, ";");

      if (ezStringUtils::FindSubString_NoCase(pAssetAttribute->GetTypeFilter(), sTypeFilter) != nullptr)
        return true;
    }
  }

  return false;
}

void ezQtAssetPropertyWidget::OnInit()
{
  EZ_ASSERT_DEV(m_pProp->GetAttributeByType<ezAssetBrowserAttribute>() != nullptr, "ezQtAssetPropertyWidget was created without a ezAssetBrowserAttribute!");
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

    pThumbnailPixmap = ezQtImageCache::GetSingleton()->QueryPixmapForType(
      sTypeFilter, szThumbnailPath, QModelIndex(), QVariant(uiUserData1), QVariant(uiUserData2), &m_uiThumbnailID);
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

        m_pWidget->setText(ezMakeQString(sText));

        m_pButton->setIcon(QIcon());
        m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);

        m_Pal.setColor(QPalette::Text, Qt::red);
        m_pWidget->setPalette(m_Pal);

        return;
      }

      ezUuid newAssetGuid = ezConversionUtils::ConvertStringToUuid(sText);

      // If this is a thumbnail or transform dependency, make sure the target is not in our inverse hull, i.e. we don't create a circular dependency.
      const ezAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<ezAssetBrowserAttribute>();
      if (pAssetAttribute->GetDependencyFlags().IsAnySet(ezDependencyFlags::Thumbnail | ezDependencyFlags::Transform))
      {
        ezUuid documentGuid = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetGuid();
        ezAssetCurator::ezLockedSubAsset asset = ezAssetCurator::GetSingleton()->GetSubAsset(documentGuid);
        if (asset.isValid())
        {
          ezSet<ezUuid> inverseHull;
          ezAssetCurator::GetSingleton()->GenerateInverseTransitiveHull(asset->m_pAssetInfo, inverseHull, true, true);
          if (inverseHull.Contains(newAssetGuid))
          {
            ezQtUiServices::GetSingleton()->MessageBoxWarning("This asset can't be used here, as that would create a circular dependency.");
            return;
          }
        }
      }

      m_AssetGuid = newAssetGuid;
      auto pAsset = ezAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid);

      if (pAsset)
      {
        pAsset->GetSubAssetIdentifier(sText);

        sThumbnailPath = pAsset->m_pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pAsset->m_pAssetInfo->m_Path, pAsset->m_Data.m_sName);
      }
      else
      {
        m_AssetGuid = ezUuid();
      }
    }

    UpdateThumbnail(m_AssetGuid, sThumbnailPath);

    {
      const QColor validColor = ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Green));
      const QColor invalidColor = ezToQtColor(ezColorScheme::LightUI(ezColorScheme::Red));

      m_Pal.setColor(QPalette::Text, m_AssetGuid.IsValid() ? validColor : invalidColor);
      m_pWidget->setPalette(m_Pal);

      if (m_AssetGuid.IsValid())
        m_pWidget->setToolTip(QStringLiteral("Valid asset selected.\n\nCTRL+LMB or MMB to open the asset document.\nSHIFT+LMB to select a different asset."));
      else
        m_pWidget->setToolTip(QStringLiteral("The selected file is not a valid asset."));
    }

    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(sText.GetData()));
  }
}

void ezQtAssetPropertyWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  m_pWidget->setPalette(m_Pal);
  ezQtStandardPropertyWidget::showEvent(event);
}

void ezQtAssetPropertyWidget::FillAssetMenu(QMenu& menu)
{
  if (!menu.isEmpty())
    menu.addSeparator();

  const bool bAsset = m_AssetGuid.IsValid();
  menu.setDefaultAction(menu.addAction(QIcon(), QLatin1String("Select Asset"), this, SLOT(on_BrowseFile_clicked())));
  menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("Open Asset"), this, SLOT(OnOpenAssetDocument()))->setEnabled(bAsset);
  menu.addAction(QIcon(), QLatin1String("Select in Asset Browser"), this, SLOT(OnSelectInAssetBrowser()))->setEnabled(bAsset);
  menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnOpenExplorer()))->setEnabled(bAsset);
  menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Guid.svg")), QLatin1String("Copy Asset Guid"), this, SLOT(OnCopyAssetGuid()))->setEnabled(bAsset);
  menu.addAction(QIcon(), QLatin1String("Create New Asset"), this, SLOT(OnCreateNewAsset()));
  menu.addAction(QIcon(":/GuiFoundation/Icons/Clear.svg"), QLatin1String("Clear Asset Reference"), this, SLOT(OnClearReference()))->setEnabled(bAsset);
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

void ezQtAssetPropertyWidget::OnOpenAssetDocument()
{
  if (!m_AssetGuid.IsValid())
    return;

  if (auto asset = ezAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid))
  {
    ezQtEditorApp::GetSingleton()->OpenDocumentQueued(asset->m_pAssetInfo->m_Path.GetAbsolutePath(), GetSelection()[0].m_pObject);
  }
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
    sPath = ezAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid)->m_pAssetInfo->m_Path.GetAbsolutePath();
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

  ezQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(ezFmt("Copied asset GUID: {}", sGuid), ezTime::MakeFromSeconds(5));
}

void ezQtAssetPropertyWidget::OnCreateNewAsset()
{
  ezString sPath;

  // try to pick a good path
  {
    if (m_AssetGuid.IsValid())
    {
      sPath = ezAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid)->m_pAssetInfo->m_Path.GetAbsolutePath();
    }
    else
    {
      sPath = m_pWidget->text().toUtf8().data();

      if (sPath.IsEmpty())
      {
        sPath = ":project/";
      }

      ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
    }
  }

  const ezAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<ezAssetBrowserAttribute>();
  ezStringBuilder sTypeFilter = pAssetAttribute->GetTypeFilter();

  ezHybridArray<ezString, 4> allowedTypes;
  sTypeFilter.Split(false, allowedTypes, ";");

  ezStringBuilder tmp;

  for (ezString& type : allowedTypes)
  {
    tmp = type;
    tmp.Trim(" ");
    type = tmp;
  }

  struct info
  {
    ezAssetDocumentManager* pAssetMan = nullptr;
    const ezDocumentTypeDescriptor* pDocType = nullptr;
  };

  ezMap<ezString, info> typesToUse;

  {
    const ezHybridArray<ezDocumentManager*, 16>& managers = ezDocumentManager::GetAllDocumentManagers();

    for (ezDocumentManager* pMan : managers)
    {
      if (auto pAssetMan = ezDynamicCast<ezAssetDocumentManager*>(pMan))
      {
        ezHybridArray<const ezDocumentTypeDescriptor*, 4> documentTypes;
        pAssetMan->GetSupportedDocumentTypes(documentTypes);

        for (const ezDocumentTypeDescriptor* pType : documentTypes)
        {
          if (allowedTypes.IndexOf(pType->m_sDocumentTypeName) == ezInvalidIndex)
          {
            for (const ezString& compType : pType->m_CompatibleTypes)
            {
              if (allowedTypes.IndexOf(compType) != ezInvalidIndex)
                goto allowed;
            }

            continue;
          }

        allowed:

          auto& toUse = typesToUse[pType->m_sDocumentTypeName];

          toUse.pAssetMan = pAssetMan;
          toUse.pDocType = pType;
        }
      }
    }
  }

  if (typesToUse.IsEmpty())
    return;

  ezStringBuilder sFilter;
  QString sSelectedFilter;

  for (auto it : typesToUse)
  {
    const auto& ttu = it.Value();

    const ezString sAssetType = ttu.pDocType->m_sDocumentTypeName;
    const ezString sExtension = ttu.pDocType->m_sFileExtension;

    sFilter.AppendWithSeparator(";;", sAssetType, " (*.", sExtension, ")");

    if (sSelectedFilter.isEmpty())
    {
      sSelectedFilter = sExtension.GetData();
    }
  }


  ezStringBuilder sOutput = sPath;
  {

    QString sStartDir = sOutput.GetFileDirectory().GetData(tmp);
    sOutput = QFileDialog::getSaveFileName(
      QApplication::activeWindow(), "Create Asset", sStartDir, sFilter.GetData(), &sSelectedFilter, QFileDialog::Option::DontResolveSymlinks)
                .toUtf8()
                .data();

    if (sOutput.IsEmpty())
      return;
  }

  sFilter = sOutput.GetFileExtension();

  for (auto it : typesToUse)
  {
    const auto& ttu = it.Value();

    if (sFilter.IsEqual_NoCase(ttu.pDocType->m_sFileExtension))
    {
      ezDocument* pDoc = nullptr;

      const ezStatus res = ttu.pAssetMan->CreateDocument(ttu.pDocType->m_sDocumentTypeName, sOutput, pDoc, ezDocumentFlags::RequestWindow | ezDocumentFlags::AddToRecentFilesList);

      ezQtUiServices::GetSingleton()->MessageBoxStatus(res, "Creating the document failed.");

      if (res.Succeeded())
      {
        // if this is an asset, make sure it gets transformed, so that the output file exists
        // and make sure the filesystem knows about it (the asset lookup table is written)
        // so that redirections inside the resource manager will work right away
        // otherwise they may only work after a while (the world gets set up again) which would be irritating
        if (ezAssetDocument* pAsset = ezDynamicCast<ezAssetDocument*>(pDoc))
        {
          ezAssetCurator::GetSingleton()->NotifyOfAssetChange(pAsset->GetGuid());

          if (pAsset->TransformAsset(ezTransformFlags::Default).Failed())
          {
            ezLog::Error("Failed to transform newly created asset '{}'", pDoc->GetDocumentPath());
            break;
          }

          ezAssetCurator::GetSingleton()->MainThreadTick(false);
          ezAssetCurator::GetSingleton()->WriteAssetTables(nullptr, true).IgnoreResult();
        }

        pDoc->EnsureVisible();

        InternalSetValue(sOutput.GetData());
        on_TextFinished_triggered();
      }
      break;
    }
  }
}

void ezQtAssetPropertyWidget::OnClearReference()
{
  InternalSetValue("");
  on_TextFinished_triggered();
}

void ezQtAssetPropertyWidget::OnShowMenu()
{
  m_pButton->menu()->clear();
  FillAssetMenu(*m_pButton->menu());
}

void ezQtAssetPropertyWidget::on_BrowseFile_clicked()
{
  ezStringBuilder sFile = m_pWidget->text().toUtf8().data();
  const ezAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<ezAssetBrowserAttribute>();

  ezQtAssetBrowserDlg dlg(this, m_AssetGuid, pAssetAttribute->GetTypeFilter(), {}, pAssetAttribute->GetRequiredTag());
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
