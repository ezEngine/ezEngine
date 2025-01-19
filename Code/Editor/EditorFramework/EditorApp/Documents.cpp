#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <ToolsFoundation/Document/DocumentUtils.h>

void ezQtEditorApp::OpenDocumentQueued(ezStringView sDocument, const ezDocumentObject* pOpenContext /*= nullptr*/)
{
  QMetaObject::invokeMethod(this, "SlotQueuedOpenDocument", Qt::ConnectionType::QueuedConnection, Q_ARG(QString, ezMakeQString(sDocument)), Q_ARG(void*, (void*)pOpenContext));
}

ezDocument* ezQtEditorApp::OpenDocument(ezStringView sDocument, ezBitflags<ezDocumentFlags> flags, const ezDocumentObject* pOpenContext)
{
  EZ_PROFILE_SCOPE("OpenDocument");

  if (IsInHeadlessMode())
    flags.Remove(ezDocumentFlags::RequestWindow);

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;

  if (ezDocumentManager::FindDocumentTypeFromPath(sDocument, false, pTypeDesc).Failed())
  {
    ezStringBuilder sTemp;
    sTemp.SetFormat("The selected file extension '{0}' is not registered with any known type.\nCannot open file '{1}'", ezPathUtils::GetFileExtension(sDocument), sDocument);
    ezQtUiServices::MessageBoxWarning(sTemp);
    return nullptr;
  }

  // does the same document already exist and is open ?
  ezDocument* pDocument = pTypeDesc->m_pManager->GetDocumentByPath(sDocument);
  if (!pDocument)
  {
    ezStatus res = pTypeDesc->m_pManager->CanOpenDocument(sDocument);
    if (res.Succeeded())
    {
      res = pTypeDesc->m_pManager->OpenDocument(pTypeDesc->m_sDocumentTypeName, sDocument, pDocument, flags, pOpenContext);
    }

    if (res.Failed())
    {
      ezStringBuilder s;
      s.SetFormat("Failed to open document: \n'{0}'", sDocument);
      ezQtUiServices::MessageBoxStatus(res, s);
      return nullptr;
    }

    EZ_ASSERT_DEV(pDocument != nullptr, "Opening of document type '{0}' succeeded, but returned pointer is nullptr", pTypeDesc->m_sDocumentTypeName);

    if (pDocument->GetUnknownObjectTypeInstances() > 0)
    {
      ezStringBuilder s;
      s.SetFormat("The document '{}' contained {} objects of an unknown type. Necessary plugins may be missing.\n\n\
If you save this document, all data for these objects is lost permanently!\n\n\
The following types are missing:\n",
        sDocument, pDocument->GetUnknownObjectTypeInstances());

      for (auto it = pDocument->GetUnknownObjectTypes().GetIterator(); it.IsValid(); ++it)
      {
        s.AppendFormat(" '{0}' ", (*it));
      }
      ezQtUiServices::MessageBoxWarning(s);
    }
  }

  if (flags.IsSet(ezDocumentFlags::RequestWindow))
  {
    ezQtContainerWindow::EnsureVisibleAnyContainer(pDocument).IgnoreResult();
  }

  return pDocument;
}

ezDocument* ezQtEditorApp::CreateDocument(ezStringView sDocument, ezBitflags<ezDocumentFlags> flags, const ezDocumentObject* pOpenContext)
{
  EZ_PROFILE_SCOPE("CreateDocument");

  if (IsInHeadlessMode())
    flags.Remove(ezDocumentFlags::RequestWindow);

  const ezDocumentTypeDescriptor* pTypeDesc = nullptr;

  {
    ezStatus res = ezDocumentUtils::IsValidSaveLocationForDocument(sDocument, &pTypeDesc);
    if (res.Failed())
    {
      ezStringBuilder s;
      s.SetFormat("Failed to create document: \n'{0}'", sDocument);
      ezQtUiServices::MessageBoxStatus(res, s);
      return nullptr;
    }
  }

  ezDocument* pDocument = nullptr;
  {
    ezStatus result = pTypeDesc->m_pManager->CreateDocument(pTypeDesc->m_sDocumentTypeName, sDocument, pDocument, flags, pOpenContext);
    if (result.Failed())
    {
      ezStringBuilder s;
      s.SetFormat("Failed to create document: \n'{0}'", sDocument);
      ezQtUiServices::MessageBoxStatus(result, s);
      return nullptr;
    }

    EZ_ASSERT_DEV(pDocument != nullptr, "Creation of document type '{0}' succeeded, but returned pointer is nullptr", pTypeDesc->m_sDocumentTypeName);
    EZ_ASSERT_DEV(pDocument->GetUnknownObjectTypeInstances() == 0, "Newly created documents should not contain unknown types.");
  }


  if (flags.IsSet(ezDocumentFlags::RequestWindow))
  {
    ezQtContainerWindow::EnsureVisibleAnyContainer(pDocument).IgnoreResult();
  }

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

    default:
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
        m_RecentDocuments.Insert(r.m_pDocument->GetDocumentPath(), 0);
        if (!m_bLoadingProjectInProgress)
        {
          SaveOpenDocumentsList();
        }
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
        m_RecentDocuments.Insert(r.m_pDocument->GetDocumentPath(), 0);
      }
    }
    break;

    default:
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
      if (r.m_RequestStatus.Failed())
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
          if (res.Failed())
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
