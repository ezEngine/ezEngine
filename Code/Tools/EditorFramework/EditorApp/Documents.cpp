#include <PCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Profiling/Profiling.h>

void ezQtEditorApp::OpenDocumentQueued(const char* szDocument, const ezDocumentObject* pOpenContext /*= nullptr*/)
{
  QMetaObject::invokeMethod(this, "SlotQueuedOpenDocument", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, szDocument),
                            Q_ARG(void*, (void*)pOpenContext));
}

ezDocument* ezQtEditorApp::OpenDocument(const char* szDocument, ezBitflags<ezDocumentFlags> flags, const ezDocumentObject* pOpenContext)
{
  EZ_PROFILE_SCOPE("OpenDocument");

  if (m_bHeadless)
    flags.Remove(ezDocumentFlags::RequestWindow);

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;

  if (ezDocumentManager::FindDocumentTypeFromPath(szDocument, false, pTypeDesc).Failed())
  {
    ezStringBuilder sTemp;
    sTemp.Format("The selected file extension '{0}' is not registered with any known type.\nCannot open file '{1}'",
      ezPathUtils::GetFileExtension(szDocument), szDocument);
    ezQtUiServices::MessageBoxWarning(sTemp);
    return nullptr;
  }

  // does the same document already exist and is open ?
  ezDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(szDocument);
  if (!pDocument)
  {
    ezStatus res = pTypeDesc->m_pManager->CanOpenDocument(szDocument);
    if (res.m_Result.Succeeded())
    {
      res = pTypeDesc->m_pManager->OpenDocument(pTypeDesc->m_sDocumentTypeName, szDocument, pDocument, flags, pOpenContext);
    }

    if (res.m_Result.Failed())
    {
      ezStringBuilder s;
      s.Format("Failed to open document: \n'{0}'", szDocument);
      ezQtUiServices::MessageBoxStatus(res, s);
      return nullptr;
    }

    EZ_ASSERT_DEV(pDocument != nullptr, "Opening of document type '{0}' succeeded, but returned pointer is nullptr",
      pTypeDesc->m_sDocumentTypeName);

    if (pDocument->GetUnknownObjectTypeInstances() > 0)
    {
      ezStringBuilder s;
      s.Format("The document contained {0} objects of an unknown type. Necessary plugins may be missing.\n\n\
If you save this document, all data for these objects is lost permanently!\n\n\
The following types are missing:\n",
pDocument->GetUnknownObjectTypeInstances());

      for (auto it = pDocument->GetUnknownObjectTypes().GetIterator(); it.IsValid(); ++it)
      {
        s.AppendFormat(" '{0}' ", (*it));
      }
      ezQtUiServices::MessageBoxWarning(s);
    }
  }

  if (flags.IsSet(ezDocumentFlags::RequestWindow))
    ezQtContainerWindow::EnsureVisibleAnyContainer(pDocument);

  return pDocument;
}

ezDocument* ezQtEditorApp::CreateDocument(const char* szDocument, ezBitflags<ezDocumentFlags> flags, const ezDocumentObject* pOpenContext)
{
  EZ_PROFILE_SCOPE("CreateDocument");

  if (m_bHeadless)
    flags.Remove(ezDocumentFlags::RequestWindow);

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;

  if (ezDocumentManager::FindDocumentTypeFromPath(szDocument, true, pTypeDesc).Failed())
  {
    ezStringBuilder sTemp;
    sTemp.Format("The selected file extension '{0}' is not registered with any known type.\nCannot create file '{1}'",
      ezPathUtils::GetFileExtension(szDocument), szDocument);
    ezQtUiServices::MessageBoxWarning(sTemp);
    return nullptr;
  }

  // does the same document already exist and is open ?
  ezDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(szDocument);
  if (!pDocument)
  {
    ezStatus res = pTypeDesc->m_pManager->CreateDocument(pTypeDesc->m_sDocumentTypeName, szDocument, pDocument, flags);
    if (res.m_Result.Failed())
    {
      ezStringBuilder s;
      s.Format("Failed to create document: \n'{0}'", szDocument);
      ezQtUiServices::MessageBoxStatus(res, s);
      return nullptr;
    }

    EZ_ASSERT_DEV(pDocument != nullptr, "Creation of document type '{0}' succeeded, but returned pointer is nullptr",
      pTypeDesc->m_sDocumentTypeName);
    EZ_ASSERT_DEV(pDocument->GetUnknownObjectTypeInstances() == 0, "Newly created documents should not contain unknown types.");
  }
  else
  {
    ezQtUiServices::MessageBoxInformation(
      "The selected document is already open. You need to close the document before you can re-create it.");
    return nullptr;
  }

  if (flags.IsSet(ezDocumentFlags::RequestWindow))
    ezQtContainerWindow::EnsureVisibleAnyContainer(pDocument);

  return pDocument;
}

void ezQtEditorApp::SlotQueuedOpenDocument(QString sProject, void* pOpenContext)
{
  OpenDocument(sProject.toUtf8().data(), ezDocumentFlags::RequestWindow | ezDocumentFlags::AddToRecentFilesList, static_cast<const ezDocumentObject*>(pOpenContext));
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
