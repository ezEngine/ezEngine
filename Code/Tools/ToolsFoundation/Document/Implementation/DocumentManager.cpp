#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/Configuration/Plugin.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentManager, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, DocumentManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezPlugin::s_PluginEvents.AddEventHandler(ezDocumentManager::OnPluginEvent);
  }
 
  ON_CORE_SHUTDOWN
  {
    ezPlugin::s_PluginEvents.RemoveEventHandler(ezDocumentManager::OnPluginEvent);
  }

EZ_END_SUBSYSTEM_DECLARATION

ezSet<const ezRTTI*> ezDocumentManager::s_KnownManagers;
ezHybridArray<ezDocumentManager*, 16> ezDocumentManager::s_AllDocumentManagers;
ezEvent<const ezDocumentManager::Event&> ezDocumentManager::s_Events;
ezEvent<ezDocumentManager::Request&> ezDocumentManager::s_Requests;

void ezDocumentManager::OnPluginEvent(const ezPlugin::PluginEvent& e)
{
  switch (e.m_EventType)
  {
  case ezPlugin::PluginEvent::BeforeUnloading:
    UpdateBeforeUnloadingPlugins(e);
    break;
  case ezPlugin::PluginEvent::AfterPluginChanges:
    UpdatedAfterLoadingPlugins();
    break;
  }
}

void ezDocumentManager::UpdateBeforeUnloadingPlugins(const ezPlugin::PluginEvent& e)
{
  bool bChanges = false;

  for (ezUInt32 i = 0; i < s_AllDocumentManagers.GetCount();)
  {
    const ezRTTI* pRtti = s_AllDocumentManagers[i]->GetDynamicRTTI();

    if (ezStringUtils::IsEqual(pRtti->GetPluginName(), e.m_pPluginObject->GetPluginName()))
    {
      s_KnownManagers.Remove(pRtti);

      pRtti->GetAllocator()->Deallocate(s_AllDocumentManagers[i]);
      s_AllDocumentManagers.RemoveAtSwap(i);

      bChanges = true;
    }
    else
      ++i;
  }

  if (bChanges)
  {
    Event e;
    e.m_Type = Event::Type::DocumentTypesRemoved;
    s_Events.Broadcast(e);
  }
}

void ezDocumentManager::UpdatedAfterLoadingPlugins()
{
  bool bChanges = false;

  ezRTTI* pRtti = ezRTTI::GetFirstInstance();

  while (pRtti)
  {
    // find all types derived from ezDocumentManager
    if (pRtti->IsDerivedFrom<ezDocumentManager>())
    {
      // add the ones that we don't know yet
      if (!s_KnownManagers.Find(pRtti).IsValid())
      {
        // add it as 'known' even if we cannot allocate it
        s_KnownManagers.Insert(pRtti);

        if (pRtti->GetAllocator()->CanAllocate())
        {
          // create one instance of each manager type
          ezDocumentManager* pManager = (ezDocumentManager*) pRtti->GetAllocator()->Allocate();
          s_AllDocumentManagers.PushBack(pManager);

          bChanges = true;
        }
      }
    }

    pRtti = pRtti->GetNextInstance();
  }

  if (bChanges)
  {
    Event e;
    e.m_Type = Event::Type::DocumentTypesAdded;
    s_Events.Broadcast(e);
  }
}

void ezDocumentManager::GetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const
{
  out_DocumentTypes.Clear();
  InternalGetSupportedDocumentTypes(out_DocumentTypes);

}

ezStatus ezDocumentManager::CanOpenDocument(const char* szFilePath) const
{
  ezHybridArray<ezDocumentTypeDescriptor, 4> DocumentTypes;
  GetSupportedDocumentTypes(DocumentTypes);

  ezStringBuilder sPath = szFilePath;
  ezStringBuilder sExt = sPath.GetFileExtension();

  // check whether the file extension is in the list of possible extensions
  // if not, we can definitely not open this file
  for (ezUInt32 i = 0; i < DocumentTypes.GetCount(); ++i)
  {
    for (ezUInt32 e = 0; e < DocumentTypes[i].m_sFileExtensions.GetCount(); ++e)
    {
      if (DocumentTypes[i].m_sFileExtensions[e].IsEqual_NoCase(sExt))
      {
        return InternalCanOpenDocument(DocumentTypes[i].m_sDocumentTypeName, szFilePath);
      }
    }
  }

  return ezStatus("File extension is not handled by any registered type");
}

void ezDocumentManager::EnsureWindowRequested(ezDocument* pDocument)
{
  if (pDocument->m_bWindowRequested)
    return;

  pDocument->m_bWindowRequested = true;

  Event e;
  e.m_pDocument = pDocument;
  e.m_Type = Event::Type::DocumentWindowRequested;
  s_Events.Broadcast(e);

  e.m_pDocument = pDocument;
  e.m_Type = Event::Type::AfterDocumentWindowRequested;
  s_Events.Broadcast(e);
}

ezStatus ezDocumentManager::CreateOrOpenDocument(bool bCreate, const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument, bool bRequestWindow, bool bAddToRecentFilesList)
{
  ezStringBuilder sPath = szPath;
  sPath.MakeCleanPath();

  Request r;
  r.m_Type = Request::Type::DocumentAllowedToOpen;
  r.m_RequestStatus.m_Result = EZ_SUCCESS;
  r.m_sDocumentType = szDocumentTypeName;
  r.m_sDocumentPath = sPath;
  s_Requests.Broadcast(r);

  // if for example no project is open, or not the correct one, then a document cannot be opened
  if (r.m_RequestStatus.m_Result.Failed())
    return r.m_RequestStatus;

  out_pDocument = nullptr;

  ezStatus status;

  ezHybridArray<ezDocumentTypeDescriptor, 4> DocumentTypes;
  GetSupportedDocumentTypes(DocumentTypes);

  for (ezUInt32 i = 0; i < DocumentTypes.GetCount(); ++i)
  {
    if (DocumentTypes[i].m_sDocumentTypeName == szDocumentTypeName)
    {
      EZ_ASSERT_DEV(DocumentTypes[i].m_bCanCreate, "This document manager cannot create the document type '%s'", szDocumentTypeName);

      status = InternalCreateDocument(szDocumentTypeName, sPath, out_pDocument);
      EZ_ASSERT_DEV(status.m_Result == EZ_FAILURE || out_pDocument != nullptr, "Status was success, but the document manager returned a nullptr document.");

      out_pDocument->SetAddToResetFilesList(bAddToRecentFilesList);

      if (status.m_Result.Succeeded())
      {
        out_pDocument->SetupDocumentInfo(DocumentTypes[i]);

        out_pDocument->m_pDocumentManager = this;
        m_AllDocuments.PushBack(out_pDocument);

        out_pDocument->InitializeBeforeLoading();

        if (!bCreate)
          status = out_pDocument->LoadDocument();

        out_pDocument->InitializeAfterLoading();

        Event e;
        e.m_pDocument = out_pDocument;
        e.m_Type = Event::Type::DocumentOpened;

        s_Events.Broadcast(e);

        if (bRequestWindow)
          EnsureWindowRequested(out_pDocument);
      }

      return status;
    }
  }

  EZ_REPORT_FAILURE("This document manager does not support the document type '%s'", szDocumentTypeName);
  return status;
}

ezStatus ezDocumentManager::CreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument, bool bRequestWindow)
{
  return CreateOrOpenDocument(true, szDocumentTypeName, szPath, out_pDocument, bRequestWindow, true);
}

ezStatus ezDocumentManager::OpenDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument, bool bRequestWindow, bool bAddToRecentFilesList)
{
  return CreateOrOpenDocument(false, szDocumentTypeName, szPath, out_pDocument, bRequestWindow, bAddToRecentFilesList);
}

void ezDocumentManager::CloseDocument(ezDocument* pDocument)
{
  EZ_ASSERT_DEV(pDocument != nullptr, "Invalid document pointer");

  if (!m_AllDocuments.Remove(pDocument))
    return;

  pDocument->BroadcastSaveDocumentMetaState();

  Event e;
  e.m_pDocument = pDocument;
  e.m_Type = Event::Type::DocumentClosing;
  s_Events.Broadcast(e);
  
  delete pDocument;

  e.m_pDocument = nullptr;
  e.m_Type = Event::Type::DocumentClosed;
  s_Events.Broadcast(e);
}

void ezDocumentManager::CloseAllDocumentsOfManager()
{
  while (!m_AllDocuments.IsEmpty())
  {
    CloseDocument(m_AllDocuments[0]);
  }
}

void ezDocumentManager::CloseAllDocuments()
{
  for (ezDocumentManager* pMan : s_AllDocumentManagers)
  {
    pMan->CloseAllDocumentsOfManager();
  }
}

ezDocument* ezDocumentManager::GetDocumentByPath(const char* szPath) const
{
  ezStringBuilder sPath = szPath;
  sPath.MakeCleanPath();

  for (ezDocument* pDoc : m_AllDocuments)
  {
    if (sPath.IsEqual_NoCase(pDoc->GetDocumentPath()))
      return pDoc;
  }

  return nullptr;
}


ezResult ezDocumentManager::EnsureDocumentIsClosed(const char* szPath)
{
  auto pDoc = GetDocumentByPath(szPath);

  if (pDoc == nullptr)
    return EZ_SUCCESS;

  CloseDocument(pDoc);

  return EZ_FAILURE;
}

ezResult ezDocumentManager::FindDocumentTypeFromPath(const char* szPath, bool bForCreation, ezDocumentManager*& out_pTypeManager, ezDocumentTypeDescriptor* out_pTypeDesc)
{
  const ezString sFileExt = ezPathUtils::GetFileExtension(szPath);

  out_pTypeManager = nullptr;

  for (ezDocumentManager* pMan : ezDocumentManager::GetAllDocumentManagers())
  {
    ezHybridArray<ezDocumentTypeDescriptor, 4> Types;
    pMan->GetSupportedDocumentTypes(Types);

    for (const ezDocumentTypeDescriptor& desc : Types)
    {
      if (bForCreation && !desc.m_bCanCreate)
        continue;

      for (const ezString& ext : desc.m_sFileExtensions)
      {
        if (ext.IsEqual_NoCase(sFileExt))
        {
          out_pTypeManager = pMan;

          if (out_pTypeDesc != nullptr)
            *out_pTypeDesc = desc;

          return EZ_SUCCESS;
        }
      }
    }
  }

  return EZ_FAILURE;
}

// todo on close doc: remove from m_AllDocuments

