#include <EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <QFileDialog>


void ezQtEditorApp::GuiCreateProject()
{
  QMetaObject::invokeMethod(this, "SlotQueuedGuiCreateOrOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(bool, true));
}

void ezQtEditorApp::GuiOpenProject()
{
  QMetaObject::invokeMethod(this, "SlotQueuedGuiCreateOrOpenProject", Qt::ConnectionType::QueuedConnection, Q_ARG(bool, false));
}

void ezQtEditorApp::SlotQueuedGuiCreateOrOpenProject(bool bCreate)
{
  GuiCreateOrOpenProject(bCreate);
}

void ezQtEditorApp::GuiCreateOrOpenProject(bool bCreate)
{
  const QString sDir = QString::fromUtf8(m_sLastProjectFolder.GetData());
  ezStringBuilder sFile;

  const char* szFilter = "ezProject (ezProject)";

  if (bCreate)
    sFile = QFileDialog::getExistingDirectory(QApplication::activeWindow(), QLatin1String("Choose Folder for New Project"), sDir,
                                              QFileDialog::Option::DontResolveSymlinks)
                .toUtf8()
                .data();
  else
    sFile = QFileDialog::getOpenFileName(QApplication::activeWindow(), QLatin1String("Open Project"), sDir, QLatin1String(szFilter),
                                         nullptr, QFileDialog::Option::DontResolveSymlinks)
                .toUtf8()
                .data();

  if (sFile.IsEmpty())
    return;

  if (bCreate)
    sFile.AppendPath("ezProject");

  m_sLastProjectFolder = ezPathUtils::GetFileDirectory(sFile);

  CreateOrOpenProject(bCreate, sFile);
}
