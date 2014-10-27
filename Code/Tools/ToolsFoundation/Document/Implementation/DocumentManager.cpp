#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <Foundation/Configuration/SubSystem.h>
#include <Foundation/Configuration/Plugin.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentManagerBase, ezReflectedClass, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, DocumentManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezPlugin::s_PluginEvents.AddEventHandler(ezDocumentManagerBase::OnPluginEvent);
  }
 
  ON_CORE_SHUTDOWN
  {
    ezPlugin::s_PluginEvents.RemoveEventHandler(ezDocumentManagerBase::OnPluginEvent);
  }

EZ_END_SUBSYSTEM_DECLARATION

ezSet<const ezRTTI*> ezDocumentManagerBase::s_KnownManagers;
ezHybridArray<ezDocumentManagerBase*, 16> ezDocumentManagerBase::s_AllDocumentManagers;
ezEvent<const ezDocumentManagerBase::Event&> ezDocumentManagerBase::s_Events;

void ezDocumentManagerBase::OnPluginEvent(const ezPlugin::PluginEvent& e)
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

void ezDocumentManagerBase::UpdateBeforeUnloadingPlugins(const ezPlugin::PluginEvent& e)
{
  bool bChanges = false;

  for (ezUInt32 i = 0; i < s_AllDocumentManagers.GetCount();)
  {
    const ezRTTI* pRtti = s_AllDocumentManagers[i]->GetDynamicRTTI();

    if (ezStringUtils::IsEqual(pRtti->GetPluginName(), e.m_pPluginObject->GetPluginName()))
    {
      s_KnownManagers.Erase(pRtti);

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

void ezDocumentManagerBase::UpdatedAfterLoadingPlugins()
{
  bool bChanges = false;

  ezRTTI* pRtti = ezRTTI::GetFirstInstance();

  while (pRtti)
  {
    // find all types derived from ezDocumentManagerBase
    if (pRtti->IsDerivedFrom<ezDocumentManagerBase>())
    {
      // add the ones that we don't know yet
      if (!s_KnownManagers.Find(pRtti).IsValid())
      {
        // add it as 'known' even if we cannot allocate it
        s_KnownManagers.Insert(pRtti);

        if (pRtti->GetAllocator()->CanAllocate())
        {
          // create one instance of each manager type
          ezDocumentManagerBase* pManager = (ezDocumentManagerBase*) pRtti->GetAllocator()->Allocate();
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

void ezDocumentManagerBase::GetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const
{
  out_DocumentTypes.Clear();
  InternalGetSupportedDocumentTypes(out_DocumentTypes);

  ezStringBuilder sTemp;
  for (ezUInt32 i = 0; i < out_DocumentTypes.GetCount(); ++i)
  {
    for (ezUInt32 e = 0; e < out_DocumentTypes[i].m_sFileExtensions.GetCount(); ++e)
    {
      sTemp = out_DocumentTypes[i].m_sFileExtensions[e];
      sTemp.ToLower();

      out_DocumentTypes[i].m_sFileExtensions[e] = sTemp;
    }
  }
}

bool ezDocumentManagerBase::CanOpenDocument(const char* szFilePath) const
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
      if (DocumentTypes[i].m_sFileExtensions[e].IsEqual(sExt))
      {
        return InternalCanOpenDocument(szFilePath);
      }
    }
  }

  return false;
}

ezStatus ezDocumentManagerBase::CreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocumentBase*& out_pDocument)
{
  out_pDocument = nullptr;

  ezStatus status;
  status.m_Result = EZ_FAILURE;

  ezHybridArray<ezDocumentTypeDescriptor, 4> DocumentTypes;
  GetSupportedDocumentTypes(DocumentTypes);

  for (ezUInt32 i = 0; i < DocumentTypes.GetCount(); ++i)
  {
    if (DocumentTypes[i].m_sDocumentTypeName == szDocumentTypeName)
    {
      EZ_ASSERT(DocumentTypes[i].m_bCanCreate, "This document manager cannot create the document type '%s'", szDocumentTypeName);

      status = InternalCreateDocument(szDocumentTypeName, szPath, out_pDocument);

      EZ_ASSERT(status.m_Result == EZ_FAILURE || out_pDocument != nullptr, "Status was success, but the document manager returned a nullptr document.");

      if (status.m_Result.Succeeded())
      {
        Event e;
        e.m_pDocument = out_pDocument;
        e.m_Type = Event::Type::DocumentOpened;

        s_Events.Broadcast(e);
      }

      return status;
    }
  }

  EZ_REPORT_FAILURE("This document manager does not support the document type '%s'", szDocumentTypeName);
  return status;
}

