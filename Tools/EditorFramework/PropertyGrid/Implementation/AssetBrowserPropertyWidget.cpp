#include <PCH.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>
#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

#include <QHBoxLayout>
#include <QToolButton>
#include <QLineEdit>

ezAssetBrowserPropertyWidget::ezAssetBrowserPropertyWidget() : ezPropertyEditorLineEditWidget()
{
  m_pButton = nullptr;
}

void ezAssetBrowserPropertyWidget::OnInit()
{
  ezPropertyEditorLineEditWidget::OnInit();

  const ezAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<ezAssetBrowserAttribute>();
  EZ_ASSERT_DEV(pAssetAttribute != nullptr, "ezPropertyEditorFileBrowserWidget was created without a ezAssetBrowserAttribute!");

  m_pButton = new QToolButton(this);
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  m_pButton->setText("...");

  m_pLayout->addWidget(m_pButton);

  connect(m_pButton, SIGNAL(clicked()), this, SLOT(on_BrowseFile_clicked()));

  if (m_pProp->GetAttributeByType<ezReadOnlyAttribute>() != nullptr || m_pProp->GetFlags().IsSet(ezPropertyFlags::ReadOnly))
  {
    if (m_pButton)
      m_pButton->setEnabled(false);
  }
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

