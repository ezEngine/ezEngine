#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Document/Document.h>

class EZ_TOOLSFOUNDATION_DLL ezDocumentManagerBase
{
public:
  virtual ~ezDocumentManagerBase() { }

  virtual bool CanOpen(ezDocumentInfo& docInfo) const;

  virtual const ezDocumentBase* Open(ezDocumentInfo& docInfo);

  virtual void Show(const ezDocumentBase* pDocument);

  virtual void Save(const ezDocumentBase* pDocument, ezDocumentInfo* pDocInfo = nullptr);

  virtual void Close(const ezDocumentBase* pDocument);

public:
  //ezEvent<ezDocumentChange&> m_DocumentAddedEvent;
  //ezEvent<ezDocumentChange&> m_DocumentRemovedEvent;
};
