#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Basics/Status.h>
#include <ToolsFoundation/Document/Document.h>

struct ezDocumentTypeDescriptor
{
  ezHybridArray<ezString, 4> m_sFileExtensions;
  ezString m_sDocumentTypeName;
  bool m_bCanCreate;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentManagerBase : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentManagerBase);

public:
  virtual ~ezDocumentManagerBase() { }

  static const ezHybridArray<ezDocumentManagerBase*, 16>& GetAllDocumentManagers() { return s_AllDocumentManagers; }

  void GetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const;

  bool CanOpenDocument(const char* szFilePath) const;

  ezStatus CreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocumentBase*& out_pDocument);

  //virtual bool CanOpen(ezDocumentInfo& docInfo) const;
  //virtual const ezDocumentBase* Open(ezDocumentInfo& docInfo);
  //virtual void Show(const ezDocumentBase* pDocument);
  //virtual void Save(const ezDocumentBase* pDocument, ezDocumentInfo* pDocInfo = nullptr);
  //virtual void Close(const ezDocumentBase* pDocument);

  struct Event
  {
    enum class Type
    {
      DocumentTypesRemoved,
      DocumentTypesAdded,
      DocumentOpened,
    };

    Type m_Type;
    ezDocumentBase* m_pDocument;
  };

  static ezEvent<const Event&> s_Events;

private:
  virtual bool InternalCanOpenDocument(const char* szFilePath) const = 0;
  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocumentBase*& out_pDocument) = 0;
  virtual void InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const = 0;

private:
  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, DocumentManager);

  static void OnPluginEvent(const ezPlugin::PluginEvent& e);

  static void UpdateBeforeUnloadingPlugins(const ezPlugin::PluginEvent& e);
  static void UpdatedAfterLoadingPlugins();

  static ezSet<const ezRTTI*> s_KnownManagers;
  static ezHybridArray<ezDocumentManagerBase*, 16> s_AllDocumentManagers;

  //ezEvent<ezDocumentChange&> m_DocumentAddedEvent;
  //ezEvent<ezDocumentChange&> m_DocumentRemovedEvent;
};
