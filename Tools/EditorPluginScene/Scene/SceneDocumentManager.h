#pragma once

#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Basics/Status.h>

class ezSceneDocumentManager : public ezDocumentManagerBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneDocumentManager);

public:
  ezSceneDocumentManager()
  {
    s_pSingleton = this;
  }

  static ezSceneDocumentManager* s_pSingleton;

private:
  virtual ezStatus InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const;
  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocumentBase*& out_pDocument);
  virtual void InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const;



};