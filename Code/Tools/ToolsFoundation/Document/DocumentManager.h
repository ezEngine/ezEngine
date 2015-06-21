#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Basics/Status.h>
#include <ToolsFoundation/Document/Document.h>

class EZ_TOOLSFOUNDATION_DLL ezDocumentManagerBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentManagerBase);

public:
  virtual ~ezDocumentManagerBase() { }

  static const ezHybridArray<ezDocumentManagerBase*, 16>& GetAllDocumentManagers() { return s_AllDocumentManagers; }

  static ezResult FindDocumentTypeFromPath(const char* szPath, bool bForCreation, ezDocumentManagerBase*& out_pTypeManager, ezDocumentTypeDescriptor* out_pTypeDesc = nullptr);

  void GetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const;

  ezStatus CanOpenDocument(const char* szFilePath) const;

  ezStatus CreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocumentBase*& out_pDocument, bool bRequestWindow = true);
  ezStatus OpenDocument(const char* szDocumentTypeName, const char* szPath, ezDocumentBase*& out_pDocument, bool bRequestWindow = true);
  void CloseDocument(ezDocumentBase* pDocument);
  void EnsureWindowRequested(ezDocumentBase* pDocument);

  const ezDynamicArray<ezDocumentBase*>& GetAllDocuments() const { return m_AllDocuments; }

  ezDocumentBase* GetDocumentByPath(const char* szPath) const;

  void CloseAllDocumentsOfManager();
  static void CloseAllDocuments();

  struct Event
  {
    enum class Type
    {
      DocumentTypesRemoved,
      DocumentTypesAdded,
      DocumentOpened,
      DocumentWindowRequested,
      DocumentClosing,
      DocumentClosed, // this will not point to a valid document anymore, as the document is deleted, use DocumentClosing to get the event before it is deleted
    };

    Type m_Type;
    ezDocumentBase* m_pDocument;
  };

  struct Request
  {
    enum class Type
    {
      DocumentAllowedToOpen,
    };

    Type m_Type;
    ezString m_sDocumentType;
    ezString m_sDocumentPath;
    ezStatus m_RequestStatus;
  };

  static ezEvent<const Event&> s_Events;
  static ezEvent<Request&> s_Requests;

private:
  virtual ezStatus InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const = 0;
  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocumentBase*& out_pDocument) = 0;
  virtual void InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const = 0;

private:
  ezStatus CreateOrOpenDocument(bool bCreate, const char* szDocumentTypeName, const char* szPath, ezDocumentBase*& out_pDocument, bool bRequestWindow);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, DocumentManager);

  static void OnPluginEvent(const ezPlugin::PluginEvent& e);

  static void UpdateBeforeUnloadingPlugins(const ezPlugin::PluginEvent& e);
  static void UpdatedAfterLoadingPlugins();

  ezDynamicArray<ezDocumentBase*> m_AllDocuments;

  static ezSet<const ezRTTI*> s_KnownManagers;
  static ezHybridArray<ezDocumentManagerBase*, 16> s_AllDocumentManagers;
};
