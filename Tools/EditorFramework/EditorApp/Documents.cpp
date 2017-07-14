#include <PCH.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>


void ezQtEditorApp::OpenDocument(const char* szDocument, const ezDocumentObject* pOpenContext /*= nullptr*/)
{
  QMetaObject::invokeMethod(this, "SlotQueuedOpenDocument", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, szDocument), Q_ARG(void*, (void*)pOpenContext));
}

ezDocument* ezQtEditorApp::OpenDocumentImmediate(const char* szDocument, bool bRequestWindow, bool bAddToRecentFilesList)
{
  return CreateOrOpenDocument(false, szDocument, bRequestWindow, bAddToRecentFilesList);
}

void ezQtEditorApp::SlotQueuedOpenDocument(QString sProject, void* pOpenContext)
{
  CreateOrOpenDocument(false, sProject.toUtf8().data(), true, true, static_cast<const ezDocumentObject*>(pOpenContext));
}

ezDocument* ezQtEditorApp::CreateOrOpenDocument(bool bCreate, const char* szFile, bool bRequestWindow, bool bAddToRecentFilesList, const ezDocumentObject* pOpenContext)
{
  if (m_bHeadless)
    bRequestWindow = false;

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;

  if (ezDocumentManager::FindDocumentTypeFromPath(szFile, bCreate, pTypeDesc).Failed())
  {
    ezStringBuilder sTemp = szFile;
    ezStringBuilder sExt = sTemp.GetFileExtension();

    sTemp.Format("The selected file extension '{0}' is not registered with any known type.\nCannot open file '{1}'", sExt, szFile);

    ezQtUiServices::MessageBoxWarning(sTemp);
    return nullptr;
  }

  // does the same document already exist and is open ?
  ezDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(szFile);

  if (!pDocument)
  {
    ezStatus res;

    if (bCreate)
      res = pTypeDesc->m_pManager->CreateDocument(pTypeDesc->m_sDocumentTypeName, szFile, pDocument, bRequestWindow);
    else
    {
      res = pTypeDesc->m_pManager->CanOpenDocument(szFile);

      if (res.m_Result.Succeeded())
      {
        res = pTypeDesc->m_pManager->OpenDocument(pTypeDesc->m_sDocumentTypeName, szFile, pDocument, bRequestWindow, bAddToRecentFilesList, pOpenContext);
      }
    }

    if (res.m_Result.Failed())
    {
      ezStringBuilder s;
      s.Format("Failed to open document: \n'{0}'", szFile);

      ezQtUiServices::MessageBoxStatus(res, s);
      return nullptr;
    }

    EZ_ASSERT_DEV(pDocument != nullptr, "Creation of document type '{0}' succeeded, but returned pointer is nullptr", pTypeDesc->m_sDocumentTypeName);

    if (pDocument->GetUnknownObjectTypeInstances() > 0)
    {
      ezStringBuilder s;
      s.Format("The document contained {0} objects of an unknown type. Necessary plugins may be missing.\n\n\
If you save this document, all data for these objects is lost permanently!\n\n\
The following types are missing:\n", pDocument->GetUnknownObjectTypeInstances());

      for (auto it = pDocument->GetUnknownObjectTypes().GetIterator(); it.IsValid(); ++it)
      {
        s.AppendFormat(" '{0}' ", (*it));
      }

      ezQtUiServices::MessageBoxWarning(s);
    }
  }
  else
  {
    if (bCreate)
    {
      ezQtUiServices::MessageBoxInformation("The selected document is already open. You need to close the document before you can re-create it.");
      return nullptr;
    }
  }

  if (bRequestWindow)
    ezQtContainerWindow::EnsureVisibleAnyContainer(pDocument);

  return pDocument;
}

void ezQtEditorApp::DocumentEventHandler(const ezDocumentEvent& e)
{
  switch (e.m_Type)
  {
  case ezDocumentEvent::Type::DocumentSaved:
    {
      ezPreferences::SaveDocumentPreferences(e.m_pDocument);
    }
    break;
  }
}


void ezQtEditorApp::DocumentManagerEventHandler(const ezDocumentManager::Event& r)
{
  switch (r.m_Type)
  {
  case ezDocumentManager::Event::Type::AfterDocumentWindowRequested:
    {
      if (r.m_pDocument->GetAddToRecentFilesList())
      {
        ezQtDocumentWindow* pWindow = ezQtDocumentWindow::FindWindowByDocument(r.m_pDocument);
        s_RecentDocuments.Insert(r.m_pDocument->GetDocumentPath(),
          (pWindow && pWindow->GetContainerWindow()) ? pWindow->GetContainerWindow()->GetUniqueIdentifier() : 0);
        SaveOpenDocumentsList();
      }
    }
    break;

  case ezDocumentManager::Event::Type::DocumentClosing2:
    {
      ezPreferences::SaveDocumentPreferences(r.m_pDocument);
      ezPreferences::ClearDocumentPreferences(r.m_pDocument);
    }
    break;

  case ezDocumentManager::Event::Type::DocumentClosing:
    {
      if (r.m_pDocument->GetAddToRecentFilesList())
      {
        // again, insert it into the recent documents list, such that the LAST CLOSED document is the LAST USED
        ezQtDocumentWindow* pWindow = ezQtDocumentWindow::FindWindowByDocument(r.m_pDocument);
        s_RecentDocuments.Insert(r.m_pDocument->GetDocumentPath(),
          (pWindow && pWindow->GetContainerWindow()) ? pWindow->GetContainerWindow()->GetUniqueIdentifier() : 0);
      }
    }
    break;
  }
}




void ezQtEditorApp::DocumentManagerRequestHandler(ezDocumentManager::Request& r)
{
  switch (r.m_Type)
  {
  case ezDocumentManager::Request::Type::DocumentAllowedToOpen:
    {
      // if someone else already said no, don't bother to check further
      if (r.m_RequestStatus.m_Result.Failed())
        return;

      if (!ezToolsProject::IsProjectOpen())
      {
        // if no project is open yet, try to open the corresponding one

        ezStringBuilder sProjectPath = ezToolsProject::FindProjectDirectoryForDocument(r.m_sDocumentPath);

        // if no project could be located, just reject the request
        if (sProjectPath.IsEmpty())
        {
          r.m_RequestStatus = ezStatus("No project could be opened");
          return;
        }
        else
        {
          // append the project file
          sProjectPath.AppendPath("ezProject");

          // if a project could be found, try to open it
          ezStatus res = ezToolsProject::OpenProject(sProjectPath);

          // if project opening failed, relay that error message
          if (res.m_Result.Failed())
          {
            r.m_RequestStatus = res;
            return;
          }
        }
      }
      else
      {
        if (!ezToolsProject::GetSingleton()->IsDocumentInAllowedRoot(r.m_sDocumentPath))
        {
          r.m_RequestStatus = ezStatus("The document is not part of the currently open project");
          return;
        }
      }
    }
    return;
  }
}
