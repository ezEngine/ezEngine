#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Document/Document.h>

class EZ_TOOLSFOUNDATION_DLL ezDocumentManagerBase
{
public:
  virtual bool CanOpen(ezDocumentInfo& docInfo) const;

  virtual const ezDocument* Open(ezDocumentInfo& docInfo);

  virtual void Show(const ezDocument* pDocument);

  virtual void Save(const ezDocument* pDocument, ezDocumentInfo* pDocInfo = nullptr);

  virtual void Close(const ezDocument* pDocument);

public:
  ezEvent<ezDocumentChange&> m_DocumentAddedEvent;
  ezEvent<ezDocumentChange&> m_DocumentRemovedEvent;
};
