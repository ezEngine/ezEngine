#include <EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/GameObjectReferencePropertyWidget.moc.h>
#include <QClipboard>
#include <QMenu>
#include <QMimeData>
#include <QToolButton>

ezQtGameObjectReferencePropertyWidget::ezQtGameObjectReferencePropertyWidget()
    : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  m_pLayout->setSpacing(0);
  setLayout(m_pLayout);

  m_pWidget = new QLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setFocusProxy(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered())) != nullptr,
            "signal/slot connection failed");
  EZ_VERIFY(connect(m_pWidget, SIGNAL(textChanged(const QString&)), this, SLOT(on_TextChanged_triggered(const QString&))) != nullptr,
            "signal/slot connection failed");

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("..."));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
  m_pButton->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  //EZ_VERIFY(connect(m_pButton, SIGNAL(clicked()), this, SLOT(on_BrowseFile_clicked())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pButton, &QWidget::customContextMenuRequested, this, &ezQtGameObjectReferencePropertyWidget::on_customContextMenuRequested) !=
                nullptr,
            "signal/slot connection failed");

  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);
}


void ezQtGameObjectReferencePropertyWidget::OnInit()
{
  EZ_ASSERT_DEV(m_pProp->GetAttributeByType<ezGameObjectReferenceAttribute>() != nullptr,
                "ezQtGameObjectReferencePropertyWidget was created without a ezGameObjectReferenceAttribute!");
}

void ezQtGameObjectReferencePropertyWidget::InternalSetValue(const ezVariant& value)
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

    const bool bIsGuid = ezConversionUtils::IsStringUuid(sText);
    bool bIsValidReference = bIsGuid;

    if (bIsGuid)
    {
      //if (!IsValidAssetType(sText))
      //{
      //  m_pWidget->setText(QString());
      //  m_pWidget->setPlaceholderText(QStringLiteral("<Selected Invalid Asset Type>"));

      //  m_pButton->setIcon(QIcon());
      //  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);

      //  auto pal = m_pWidget->palette();
      //  pal.setColor(QPalette::Text, Qt::red);
      //  m_pWidget->setPalette(pal);

      //  return;
      //}
    }

    {
      auto pal = m_pWidget->palette();
      pal.setColor(QPalette::Text, bIsValidReference ? QColor::fromRgb(182, 255, 0) : QColor::fromRgb(255, 170, 0));
      m_pWidget->setPalette(pal);

      if (bIsValidReference)
        m_pWidget->setToolTip(QStringLiteral("The reference is a known game object."));
      else
        m_pWidget->setToolTip(QStringLiteral("The reference is invalid."));
    }

    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(sText.GetData()));
  }
}

void ezQtGameObjectReferencePropertyWidget::FillContextMenu(QMenu& menu)
{
  if (!menu.isEmpty())
    menu.addSeparator();

  //menu.setDefaultAction(menu.addAction(QIcon(), QLatin1String("Select Asset"), this, SLOT(on_BrowseFile_clicked())));
  //menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document16.png")), QLatin1String("Open Asset"), this, SLOT(OnOpenAssetDocument()))->setEnabled(bAsset);
  //menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/DocumentGuid16.png")), QLatin1String("Copy Asset Guid"), this, SLOT(OnCopyAssetGuid()));
  menu.addAction(QIcon(":/GuiFoundation/Icons/Delete16.png"), QLatin1String("Clear Reference"), this, SLOT(OnClearReference()));
}

void ezQtGameObjectReferencePropertyWidget::on_TextFinished_triggered()
{
  ezStringBuilder sText = m_pWidget->text().toUtf8().data();

  //auto pAsset = ezAssetCurator::GetSingleton()->FindSubAsset(sText);

  //if (pAsset)
  //{
  //  ezConversionUtils::ToString(pAsset->m_Data.m_Guid, sText);
  //}

  BroadcastValueChanged(sText.GetData());
}


void ezQtGameObjectReferencePropertyWidget::on_TextChanged_triggered(const QString& value)
{
  if (!hasFocus())
    on_TextFinished_triggered();
}

void ezQtGameObjectReferencePropertyWidget::on_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  FillContextMenu(m);

  m.exec(m_pButton->mapToGlobal(pt));
}
//
//void ezQtGameObjectReferencePropertyWidget::OnCopyAssetGuid()
//{
//  ezStringBuilder sGuid;
//
//  if (m_AssetGuid.IsValid())
//  {
//    ezConversionUtils::ToString(m_AssetGuid, sGuid);
//  }
//  else
//  {
//    sGuid = m_pWidget->text().toUtf8().data();
//    if (!ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sGuid))
//      return;
//  }
//
//  QClipboard* clipboard = QApplication::clipboard();
//  QMimeData* mimeData = new QMimeData();
//  mimeData->setText(QString::fromUtf8(sGuid.GetData()));
//  clipboard->setMimeData(mimeData);
//}


void ezQtGameObjectReferencePropertyWidget::OnClearReference()
{
  InternalSetValue("");
  on_TextFinished_triggered();
}
