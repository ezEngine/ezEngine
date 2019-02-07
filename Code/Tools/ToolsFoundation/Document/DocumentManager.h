#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/Document.h>

class EZ_TOOLSFOUNDATION_DLL ezDocumentManager : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentManager, ezReflectedClass);

public:
  virtual ~ezDocumentManager() { }

  static const ezHybridArray<ezDocumentManager*, 16>& GetAllDocumentManagers() { return s_AllDocumentManagers; }

  static ezResult FindDocumentTypeFromPath(const char* szPath, bool bForCreation, const ezDocumentTypeDescriptor*& out_pTypeDesc);

  ezStatus CanOpenDocument(const char* szFilePath) const;

  ezStatus CreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument, ezBitflags<ezDocumentFlags> flags = ezDocumentFlags::None);
  ezStatus OpenDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument,
    ezBitflags<ezDocumentFlags> flags = ezDocumentFlags::AddToRecentFilesList | ezDocumentFlags::RequestWindow, const ezDocumentObject* pOpenContext = nullptr);
  void CloseDocument(ezDocument* pDocument);
  void EnsureWindowRequested(ezDocument* pDocument, const ezDocumentObject* pOpenContext = nullptr);

  const ezDynamicArray<ezDocument*>& GetAllDocuments() const { return m_AllDocuments; }

  ezDocument* GetDocumentByPath(const char* szPath) const;

  static ezDocument* GetDocumentByGuid(const ezUuid& guid);

  /// \brief If the given document is open, it will be closed. User is not asked about it, unsaved changes are discarded. Returns true if the document was open and needed to be closed.
  static bool EnsureDocumentIsClosedInAllManagers(const char* szPath);

  /// \brief If the given document is open, it will be closed. User is not asked about it, unsaved changes are discarded. Returns true if the document was open and needed to be closed.
  /// This function only operates on documents opened by this manager. Use EnsureDocumentIsClosedInAllManagers() to close documents of any type.
  bool EnsureDocumentIsClosed(const char* szPath);

  void CloseAllDocumentsOfManager();
  static void CloseAllDocuments();

  struct Event
  {
    enum class Type
    {
      DocumentTypesRemoved,
      DocumentTypesAdded,
      DocumentOpened,
      DocumentWindowRequested, ///< Sent when the window for a document is needed. Each plugin should check this and see if it can create the desired window type
      AfterDocumentWindowRequested, ///< Sent after a document window was requested. Can be used to do things after the new window has been opened
      DocumentClosing,
      DocumentClosing2, // sent after DocumentClosing but before removing the document, use this to do stuff that depends on code executed during DocumentClosing
      DocumentClosed, // this will not point to a valid document anymore, as the document is deleted, use DocumentClosing to get the event before it is deleted
    };

    Type m_Type;
    ezDocument* m_pDocument = nullptr;
    const ezDocumentObject* m_pOpenContext = nullptr;
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

  static const ezDynamicArray<const ezDocumentTypeDescriptor*>& GetAllDocumentDescriptors();

  void GetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const;

private:
  virtual ezStatus InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const { return ezStatus(EZ_SUCCESS); }
  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument,
                                          ezDocument*& out_pDocument) = 0;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const = 0;

private:
  ezStatus CreateOrOpenDocument(bool bCreate, const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument,
    ezBitflags<ezDocumentFlags> flags, const ezDocumentObject* pOpenContext = nullptr);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, DocumentManager);

  static void OnPluginEvent(const ezPlugin::PluginEvent& e);

  static void UpdateBeforeUnloadingPlugins(const ezPlugin::PluginEvent& e);
  static void UpdatedAfterLoadingPlugins();

  ezDynamicArray<ezDocument*> m_AllDocuments;

  static ezSet<const ezRTTI*> s_KnownManagers;
  static ezHybridArray<ezDocumentManager*, 16> s_AllDocumentManagers;

  static ezDynamicArray<const ezDocumentTypeDescriptor*> s_AllDocumentDescriptors;
};
