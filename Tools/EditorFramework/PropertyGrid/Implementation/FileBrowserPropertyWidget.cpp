#include <PCH.h>
#include <EditorFramework/PropertyGrid/FileBrowserPropertyWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

#include <QHBoxLayout>
#include <QToolButton>
#include <QLabel>
#include <QMenu>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileDialog>

ezQtFilePropertyWidget::ezQtFilePropertyWidget() : ezQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setMargin(0);
  setLayout(m_pLayout);

  m_pWidget = new QLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  //m_pWidget->m_pOwner = this;
  setFocusProxy(m_pWidget);

  EZ_VERIFY(connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pWidget, SIGNAL(textChanged(const QString&)), this, SLOT(on_TextChanged_triggered(const QString&))) != nullptr, "signal/slot connection failed");

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("..."));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
  m_pButton->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  EZ_VERIFY(connect(m_pButton, SIGNAL(clicked()), this, SLOT(on_BrowseFile_clicked())) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pButton, &QWidget::customContextMenuRequested, this, &ezQtFilePropertyWidget::on_customContextMenuRequested) != nullptr, "signal/slot connection failed");

  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);
}

void ezQtFilePropertyWidget::OnInit()
{
  EZ_ASSERT_DEV(m_pProp->GetAttributeByType<ezFileBrowserAttribute>() != nullptr, "ezQtFilePropertyWidget was created without a ezFileBrowserAttribute!");
}

void ezQtFilePropertyWidget::InternalSetValue(const ezVariant& value)
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

    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(sText.GetData()));
  }
}

void ezQtFilePropertyWidget::on_TextFinished_triggered()
{
  ezStringBuilder sText = m_pWidget->text().toUtf8().data();

  BroadcastValueChanged(sText.GetData());
}


void ezQtFilePropertyWidget::on_TextChanged_triggered(const QString& value)
{
  if (!hasFocus())
    on_TextFinished_triggered();
}

void ezQtFilePropertyWidget::on_customContextMenuRequested(const QPoint& pt)
{
  QMenu m;

  m.setDefaultAction(m.addAction(QIcon(), QLatin1String("Select File"), this, SLOT(on_BrowseFile_clicked())));
  m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document16.png")), QLatin1String("Open File"), this, SLOT(OnOpenFile()))->setEnabled(!m_pWidget->text().isEmpty());
  m.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder16.png")), QLatin1String("Open in Explorer"), this, SLOT(OnOpenExplorer()));

  m.exec(m_pButton->mapToGlobal(pt));
}

void ezQtFilePropertyWidget::OnOpenExplorer()
{
  ezString sPath = m_pWidget->text().toUtf8().data();
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  ezQtUiServices::OpenInExplorer(sPath, true);
}


void ezQtFilePropertyWidget::OnOpenFile()
{
  ezString sPath = m_pWidget->text().toUtf8().data();
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  if (!ezQtUiServices::OpenFileInDefaultProgram(sPath))
    ezQtUiServices::MessageBoxInformation(ezFmt("File could not be opened:\n{0}\nCheck that the file exists, that a program is associated with this file type and that access to this file is not denied.", sPath));
}

static ezMap<ezString, ezString> s_StartDirs;

void ezQtFilePropertyWidget::on_BrowseFile_clicked()
{
  ezString sFile = m_pWidget->text().toUtf8().data();
  const ezFileBrowserAttribute* pFileAttribute = m_pProp->GetAttributeByType<ezFileBrowserAttribute>();

  auto& sStartDir = s_StartDirs[pFileAttribute->GetTypeFilter()];

  if (!sFile.IsEmpty())
  {
    ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sFile);

    ezStringBuilder st = sFile;
    st = st.GetFileDirectory();

    sStartDir = st;
  }

  if (sStartDir.IsEmpty())
    sStartDir = ezToolsProject::GetSingleton()->GetProjectFile();

  QString sResult = QFileDialog::getOpenFileName(this, pFileAttribute->GetDialogTitle(), sStartDir.GetData(), pFileAttribute->GetTypeFilter(), nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sResult.isEmpty())
    return;

  sFile = sResult.toUtf8().data();
  sStartDir = sFile;

  if (!ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sFile))
  {
    ezQtUiServices::GetSingleton()->MessageBoxInformation("The selected file is not under any data directory.\nPlease select another file or copy it into one of the project's data directories.");
    return;
  }

  m_pWidget->setText(sFile.GetData());
  on_TextFinished_triggered();
}

