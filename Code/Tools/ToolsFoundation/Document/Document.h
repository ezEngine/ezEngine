#pragma once

#include <ToolsFoundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <ToolsFoundation/Basics/Guid.h>
#include <Foundation/Communication/Event.h>

struct EZ_TOOLSFOUNDATION_DLL ezDocumentInfo
{
  ezString m_sAbsoluteFilePath;
  ezGuid m_sDocumentGuid;
};

class ezDocument;
class ezCommandHistoryBase;
class ezObjectManager;
class ezDocumentObjectTree;
class ezSelectionManager;



struct EZ_TOOLSFOUNDATION_DLL ezDocumentChange
{
  const ezDocument* m_pDocument;
};

class EZ_TOOLSFOUNDATION_DLL ezDocument
{
public:
  virtual bool IsModified() const;
  virtual bool IsReadOnly() const;

  const ezCommandHistoryBase* GetCommandHistory() const;
  const ezObjectManager* GetObjectManager() const;
  const ezDocumentObjectTree* GetObjectTree() const;
  const ezSelectionManager* GetSelectionManager() const;

public:
  ezEvent<ezDocumentChange&> m_DocumentModifiedChanged;

private:
  ezCommandHistoryBase* m_pCommandHistory;
  ezObjectManager* m_pObjectManager;
  ezDocumentObjectTree* m_pObjectTree;
  ezSelectionManager* m_pSelectionManager;
};
