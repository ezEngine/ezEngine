#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/InputContexts/SelectionContext.h>
#include <EditorFramework/PropertyGrid/GameObjectReferencePropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

ezQtGameObjectReferencePropertyWidget::ezQtGameObjectReferencePropertyWidget()
  : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  m_pLayout->setSpacing(0);
  setLayout(m_pLayout);

  m_pWidget = new QLabel(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("..."));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
  m_pButton->setIcon(QIcon(":/GuiFoundation/Icons/Cursor16.png"));
  m_pButton->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  m_pButton->setCursor(Qt::WhatsThisCursor);

  EZ_VERIFY(connect(m_pButton, SIGNAL(clicked()), this, SLOT(on_PickObject_clicked())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(
    connect(m_pButton, &QWidget::customContextMenuRequested, this, &ezQtGameObjectReferencePropertyWidget::on_customContextMenuRequested) != nullptr,
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
    m_pWidget->setText(QStringLiteral("<Multiple Values>"));
  }
  else
  {
    SetValue(value.ConvertTo<ezString>().GetData());
  }
}

void ezQtGameObjectReferencePropertyWidget::FillContextMenu(QMenu& menu)
{
  if (!menu.isEmpty())
    menu.addSeparator();

  menu.setDefaultAction(
    menu.addAction(QIcon(":/GuiFoundation/Icons/Cursor16.png"), QLatin1String("Pick Object"), this, SLOT(on_PickObject_clicked())));
  QAction* pCopyAction =
    menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Copy16.png")), QLatin1String("Copy Object Reference"), this, SLOT(OnCopyReference()));
  menu.addAction(QIcon(":/GuiFoundation/Icons/Paste16.png"), QLatin1String("Paste Object Reference"), this, SLOT(OnPasteReference()));
  QAction* pSelectAction =
    menu.addAction(QIcon(":/GuiFoundation/Icons/Go16.png"), QLatin1String("Select Referenced Object"), this, SLOT(OnSelectReferencedObject()));
  QAction* pClearAction =
    menu.addAction(QIcon(":/GuiFoundation/Icons/Delete16.png"), QLatin1String("Clear Reference"), this, SLOT(OnClearReference()));

  pCopyAction->setEnabled(!m_sInternalValue.isEmpty());
  pSelectAction->setEnabled(!m_sInternalValue.isEmpty());
  // pClearAction->setEnabled(!m_sInternalValue.isEmpty()); // this would disable the clear button with multi selection
}

void ezQtGameObjectReferencePropertyWidget::PickObjectOverride(const ezDocumentObject* pObject)
{
  if (pObject != nullptr)
  {
    if (m_Items[0].m_pObject->GetDocumentObjectManager() != pObject->GetDocumentObjectManager())
    {
      if (ezQtDocumentWindow* pWindow = ezQtDocumentWindow::FindWindowByDocument(m_pGrid->GetDocument()))
      {
        pWindow->ShowTemporaryStatusBarMsg("Can't reference object in another layer.");
      }
      return;
    }

    ezStringBuilder sGuid;
    ezConversionUtils::ToString(pObject->GetGuid(), sGuid);

    SetValue(sGuid.GetData());
  }

  ClearPicking();
}

void ezQtGameObjectReferencePropertyWidget::ClearPicking()
{
  m_pGrid->GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(
    ezMakeDelegate(&ezQtGameObjectReferencePropertyWidget::SelectionManagerEventHandler, this));

  for (auto pContext : m_SelectionContextsToUnsubscribe)
  {
    pContext->ResetPickObjectOverride();
  }

  m_SelectionContextsToUnsubscribe.Clear();
}

void ezQtGameObjectReferencePropertyWidget::SelectionManagerEventHandler(const ezSelectionManagerEvent& e)
{
  // if the selection changes while we wait for a picking result, clear the picking override
  ClearPicking();
}

void ezQtGameObjectReferencePropertyWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  m_pWidget->setPalette(m_Pal);
  ezQtStandardPropertyWidget::showEvent(event);
}

void ezQtGameObjectReferencePropertyWidget::SetValue(const QString& sValue)
{
  // don't early out if the value is equal, otherwise that breaks clearing the reference with a multi-selection

  m_sInternalValue = sValue;

  const ezDocumentObject* pObject = nullptr;
  ezStringBuilder sDisplayName = m_sInternalValue.toUtf8().data();

  if (ezConversionUtils::IsStringUuid(m_sInternalValue.toUtf8().data()))
  {
    const ezUuid guid = ezConversionUtils::ConvertStringToUuid(m_sInternalValue.toUtf8().data());

    pObject = m_pObjectAccessor->GetObject(guid);
  }

  if (pObject != nullptr)
  {
    m_Pal.setColor(QPalette::WindowText, QColor::fromRgb(182, 255, 0));
    m_pWidget->setToolTip(QStringLiteral("The reference is a known game object."));

    if (auto* pGoDoc = ezDynamicCast<const ezGameObjectDocument*>(m_pGrid->GetDocument()))
    {
      pGoDoc->QueryCachedNodeName(pObject, sDisplayName, nullptr, nullptr);
    }
  }
  else
  {
    m_Pal.setColor(QPalette::WindowText, Qt::red);
    m_pWidget->setToolTip(QStringLiteral("The reference is invalid."));
  }

  m_pWidget->setPalette(m_Pal);

  m_pWidget->setText(sDisplayName.GetData());
  BroadcastValueChanged(m_sInternalValue.toUtf8().data());
}

void ezQtGameObjectReferencePropertyWidget::on_PickObject_clicked()
{
  auto dele = ezMakeDelegate(&ezQtGameObjectReferencePropertyWidget::SelectionManagerEventHandler, this);

  if (m_pGrid->GetDocument()->GetSelectionManager()->m_Events.HasEventHandler(dele))
  {
    // this happens when clicking the 'pick' button twice
    ClearPicking();
  }

  ezQtDocumentWindow* pWindow = ezQtDocumentWindow::FindWindowByDocument(m_pGrid->GetDocument());

  ezQtGameObjectDocumentWindow* pGoWindow = qobject_cast<ezQtGameObjectDocumentWindow*>(pWindow);

  if (pGoWindow == nullptr)
    return;

  for (auto pView : pGoWindow->GetViewWidgets())
  {
    if (auto pGoView = qobject_cast<ezQtGameObjectViewWidget*>(pView))
    {
      pGoView->m_pSelectionContext->SetPickObjectOverride(ezMakeDelegate(&ezQtGameObjectReferencePropertyWidget::PickObjectOverride, this));

      m_SelectionContextsToUnsubscribe.PushBack(pGoView->m_pSelectionContext);
    }
  }


  m_pGrid->GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(dele);
}

void ezQtGameObjectReferencePropertyWidget::on_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;
  m.setToolTipsVisible(true);
  FillContextMenu(m);

  m.exec(m_pButton->mapToGlobal(pt));
}

void ezQtGameObjectReferencePropertyWidget::OnSelectReferencedObject()
{
  ezStringBuilder sGuid = m_sInternalValue.toUtf8().data();

  if (!ezConversionUtils::IsStringUuid(sGuid))
    return;

  const ezUuid guid = ezConversionUtils::ConvertStringToUuid(sGuid);

  if (const ezDocumentObject* pObject = m_pObjectAccessor->GetObject(guid))
  {
    m_pGrid->GetDocument()->GetSelectionManager()->SetSelection(pObject);
  }
}

void ezQtGameObjectReferencePropertyWidget::OnCopyReference()
{
  QClipboard* clipboard = QApplication::clipboard();
  clipboard->setText(m_sInternalValue);

  ezQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(
    ezFmt("Copied Object Reference: {}", m_sInternalValue.toUtf8().data()), ezTime::Seconds(5));
}


void ezQtGameObjectReferencePropertyWidget::OnClearReference()
{
  SetValue("");
}

void ezQtGameObjectReferencePropertyWidget::OnPasteReference()
{
  QClipboard* clipboard = QApplication::clipboard();
  QString sReference = clipboard->text();

  if (ezConversionUtils::IsStringUuid(sReference.toUtf8().data()))
  {
    SetValue(sReference);
  }
}
