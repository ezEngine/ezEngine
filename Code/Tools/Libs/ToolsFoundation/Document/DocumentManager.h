#pragma once

#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class EZ_TOOLSFOUNDATION_DLL ezDocumentManager : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentManager, ezReflectedClass);

public:
  virtual ~ezDocumentManager() = default;

  static const ezHybridArray<ezDocumentManager*, 16>& GetAllDocumentManagers() { return s_AllDocumentManagers; }

  static ezResult FindDocumentTypeFromPath(ezStringView sPath, bool bForCreation, const ezDocumentTypeDescriptor*& out_pTypeDesc);

  ezStatus CanOpenDocument(ezStringView sFilePath) const;

  /// \brief Creates a new document.
  /// \param szDocumentTypeName Document type to create. See ezDocumentTypeDescriptor.
  /// \param szPath Absolute path to the document to be created.
  /// \param out_pDocument Out parameter for the resulting ezDocument. Will be nullptr on failure.
  /// \param flags Flags to define various options like whether a window should be created.
  /// \param pOpenContext An generic context object. Allows for custom data to be passed along into the construction. E.g. inform a sub-document which main document it belongs to.
  /// \return Returns the error in case the operations failed.
  ezStatus CreateDocument(
    ezStringView sDocumentTypeName, ezStringView sPath, ezDocument*& out_pDocument, ezBitflags<ezDocumentFlags> flags = ezDocumentFlags::None, const ezDocumentObject* pOpenContext = nullptr);

  /// \brief Opens an existing document.
  /// \param szDocumentTypeName Document type to open. See ezDocumentTypeDescriptor.
  /// \param szPath Absolute path to the document to be opened.
  /// \param out_pDocument Out parameter for the resulting ezDocument. Will be nullptr on failure.
  /// \param flags Flags to define various options like whether a window should be created.
  /// \param pOpenContext  An generic context object. Allows for custom data to be passed along into the construction. E.g. inform a sub-document which main document it belongs to.
  /// \return Returns the error in case the operations failed.
  /// \return Returns the error in case the operations failed.
  ezStatus OpenDocument(ezStringView sDocumentTypeName, ezStringView sPath, ezDocument*& out_pDocument,
    ezBitflags<ezDocumentFlags> flags = ezDocumentFlags::AddToRecentFilesList | ezDocumentFlags::RequestWindow,
    const ezDocumentObject* pOpenContext = nullptr);
  virtual ezStatus CloneDocument(ezStringView sPath, ezStringView sClonePath, ezUuid& inout_cloneGuid);
  void CloseDocument(ezDocument* pDocument);
  void EnsureWindowRequested(ezDocument* pDocument, const ezDocumentObject* pOpenContext = nullptr);

  /// \brief Returns a list of all currently open documents that are managed by this document manager
  const ezDynamicArray<ezDocument*>& GetAllOpenDocuments() const { return m_AllOpenDocuments; }

  ezDocument* GetDocumentByPath(ezStringView sPath) const;

  static ezDocument* GetDocumentByGuid(const ezUuid& guid);

  /// \brief If the given document is open, it will be closed. User is not asked about it, unsaved changes are discarded. Returns true if the document
  /// was open and needed to be closed.
  static bool EnsureDocumentIsClosedInAllManagers(ezStringView sPath);

  /// \brief If the given document is open, it will be closed. User is not asked about it, unsaved changes are discarded. Returns true if the document
  /// was open and needed to be closed. This function only operates on documents opened by this manager. Use EnsureDocumentIsClosedInAllManagers() to
  /// close documents of any type.
  bool EnsureDocumentIsClosed(ezStringView sPath);

  void CloseAllDocumentsOfManager();
  static void CloseAllDocuments();

  struct Event
  {
    enum class Type
    {
      DocumentTypesRemoved,
      DocumentTypesAdded,
      DocumentOpened,
      DocumentWindowRequested,      ///< Sent when the window for a document is needed. Each plugin should check this and see if it can create the desired
                                    ///< window type
      AfterDocumentWindowRequested, ///< Sent after a document window was requested. Can be used to do things after the new window has been opened
      DocumentClosing,
      DocumentClosing2,             // sent after DocumentClosing but before removing the document, use this to do stuff that depends on code executed during
                                    // DocumentClosing
      DocumentClosed,               // this will not point to a valid document anymore, as the document is deleted, use DocumentClosing to get the event before it
                                    // is deleted
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
    ezStatus m_RequestStatus = EZ_SUCCESS;
  };

  static ezCopyOnBroadcastEvent<const Event&> s_Events;
  static ezEvent<Request&> s_Requests;

  static const ezDocumentTypeDescriptor* GetDescriptorForDocumentType(ezStringView sDocumentType);
  static const ezMap<ezString, const ezDocumentTypeDescriptor*>& GetAllDocumentDescriptors();

  void GetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_documentTypes) const;

  using CustomAction = ezVariant (*)(const ezDocument*);
  static ezMap<ezString, CustomAction> s_CustomActions;

protected:
  virtual void InternalCloneDocument(ezStringView sPath, ezStringView sClonePath, const ezUuid& documentId, const ezUuid& seedGuid, const ezUuid& cloneGuid, ezAbstractObjectGraph* pHeader, ezAbstractObjectGraph* pObjects, ezAbstractObjectGraph* pTypes);

private:
  virtual void InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext) = 0;
  virtual void InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const = 0;

private:
  ezStatus CreateOrOpenDocument(bool bCreate, ezStringView sDocumentTypeName, ezStringView sPath, ezDocument*& out_pDocument,
    ezBitflags<ezDocumentFlags> flags, const ezDocumentObject* pOpenContext = nullptr);

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, DocumentManager);

  static void OnPluginEvent(const ezPluginEvent& e);

  static void UpdateBeforeUnloadingPlugins(const ezPluginEvent& e);
  static void UpdatedAfterLoadingPlugins();

  ezDynamicArray<ezDocument*> m_AllOpenDocuments;

  static ezSet<const ezRTTI*> s_KnownManagers;
  static ezHybridArray<ezDocumentManager*, 16> s_AllDocumentManagers;

  static ezMap<ezString, const ezDocumentTypeDescriptor*> s_AllDocumentDescriptors;
};
