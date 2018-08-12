#include <PCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <QFileDialog>

void ezQtEditorApp::GuiCreateOrOpenDocument(bool bCreate)
{
  const ezString sAllFilters = BuildDocumentTypeFileFilter(bCreate);

  if (sAllFilters.IsEmpty())
  {
    ezQtUiServices::MessageBoxInformation("No file types are currently known. Load plugins to add file types.");
    return;
  }

  static QString sSelectedExt;
  const QString sDir = QString::fromUtf8(m_sLastDocumentFolder.GetData());

  ezString sFile;

  if (bCreate)
    sFile = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Create Document"), sDir,
                                         QString::fromUtf8(sAllFilters.GetData()), &sSelectedExt, QFileDialog::Option::DontResolveSymlinks)
                .toUtf8()
                .data();
  else
    sFile = QFileDialog::getOpenFileName(QApplication::activeWindow(), QLatin1String("Open Document"), sDir,
                                         QString::fromUtf8(sAllFilters.GetData()), &sSelectedExt, QFileDialog::Option::DontResolveSymlinks)
                .toUtf8()
                .data();

  if (sFile.IsEmpty())
    return;

  m_sLastDocumentFolder = ezPathUtils::GetFileDirectory(sFile);

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;
  if (ezDocumentManager::FindDocumentTypeFromPath(sFile, bCreate, pTypeDesc).Succeeded())
  {
    sSelectedExt = pTypeDesc->m_sDocumentTypeName;
  }

  CreateOrOpenDocument(bCreate, sFile);
}

void ezQtEditorApp::GuiCreateDocument()
{
  GuiCreateOrOpenDocument(true);
}

void ezQtEditorApp::GuiOpenDocument()
{
  GuiCreateOrOpenDocument(false);
}


ezString ezQtEditorApp::BuildDocumentTypeFileFilter(bool bForCreation)
{
  ezStringBuilder sAllFilters;
  const char* sepsep = "";

  if (!bForCreation)
  {
    sAllFilters = "All Files (*.*)";
    sepsep = ";;";
  }

  const auto& allDesc = ezDocumentManager::GetAllDocumentDescriptors();

  for (auto desc : allDesc)
  {
    if (bForCreation && !desc->m_bCanCreate)
      continue;

    if (desc->m_sFileExtension.IsEmpty())
      continue;

    sAllFilters.Append(sepsep, desc->m_sDocumentTypeName, " (*.", desc->m_sFileExtension, ")");
    sepsep = ";;";
  }

  return sAllFilters;
}


void ezQtEditorApp::DocumentWindowEventHandler(const ezQtDocumentWindowEvent& e)
{
  switch (e.m_Type)
  {
    case ezQtDocumentWindowEvent::WindowClosed:
    {
      // if all windows are closed, show at least the settings window
      if (ezQtDocumentWindow::GetAllDocumentWindows().GetCount() == 0)
      {
        ShowSettingsDocument();
      }
    }
    break;
  }
}
