#pragma once

#include <ToolsFoundation/Document/Document.h>

class ezTestDocument : public ezDocumentBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestDocument);

public:
  ezTestDocument(const char* szDocumentPath);
  ~ezTestDocument();

protected:
  virtual ezStatus InternalSaveDocument() override 
  {
    //return "Saving is not implemented"; 
    return ezStatus(EZ_SUCCESS);
  }

  virtual ezStatus InternalLoadDocument() override
  {
    return ezStatus(EZ_SUCCESS);
  }
};
