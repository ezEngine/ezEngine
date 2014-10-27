#pragma once

#include <ToolsFoundation/Document/DocumentManager.h>

class ezTestDocumentManager : public ezDocumentManagerBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestDocumentManager);

public:
  ezTestDocumentManager()
  {
    s_pSingleton = this;
  }

  static ezTestDocumentManager* s_pSingleton;

private:
  virtual bool InternalCanOpenDocument(const char* szFilePath) const;
  virtual ezStatus InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocumentBase*& out_pDocument);
  virtual void InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const;



};