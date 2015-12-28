#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Document.h>

class ezDuplicateObjectsCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDuplicateObjectsCommand, ezCommand);

public:
  ezDuplicateObjectsCommand();

public: // Properties
  ezString m_sJsonGraph;
  ezString m_sParentNodes;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  struct DuplicatedObject
  {
    ezDocumentObject* m_pObject;
    ezDocumentObject* m_pParent;
    ezString m_sParentProperty;
    ezVariant m_Index;
  };

  ezHybridArray<DuplicatedObject, 4> m_DuplicatedObjects;
};

