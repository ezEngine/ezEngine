#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Document.h>

class EZ_TOOLSFOUNDATION_DLL ezAddObjectCommand : public ezCommandBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAddObjectCommand);

public:
  ezAddObjectCommand();

public: // Properties
  void SetType(const char* szType);
  const char* GetType() const;

  const ezRTTI* m_pType;
  ezUuid m_Parent;
  ezInt32 m_iChildIndex;
  ezUuid m_NewObjectGuid; ///< This is optional. If not filled out, a new guid is assigned automatically.

private:
  virtual ezStatus Do(bool bRedo) override;
  virtual ezStatus Undo(bool bFireEvents) override;
  virtual void Cleanup(CommandState state) override;

private:
  ezDocumentObjectBase* m_pObject;
};

class EZ_TOOLSFOUNDATION_DLL ezRemoveObjectCommand : public ezCommandBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRemoveObjectCommand);

public:
  ezRemoveObjectCommand();

public: // Properties
  ezUuid m_Object;

private:
  virtual ezStatus Do(bool bRedo) override;
  virtual ezStatus Undo(bool bFireEvents) override;
  virtual void Cleanup(CommandState state) override;

private:
  ezDocumentObjectBase* m_pParent;
  ezInt32 m_iChildIndex;
  ezDocumentObjectBase* m_pObject;
};


class EZ_TOOLSFOUNDATION_DLL ezMoveObjectCommand : public ezCommandBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMoveObjectCommand);

public:
  ezMoveObjectCommand();

public: // Properties
  ezUuid m_Object;
  ezUuid m_NewParent;
  ezInt32 m_iNewChildIndex;

private:
  virtual ezStatus Do(bool bRedo) override;
  virtual ezStatus Undo(bool bFireEvents) override;
  virtual void Cleanup(CommandState state) override { }

private:
  ezDocumentObjectBase* m_pObject;
  ezDocumentObjectBase* m_pOldParent;
  ezDocumentObjectBase* m_pNewParent;
  ezInt32 m_iOldChildIndex;
};

class EZ_TOOLSFOUNDATION_DLL ezSetObjectPropertyCommand : public ezCommandBase
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSetObjectPropertyCommand);

public:
  ezSetObjectPropertyCommand();

public: // Properties
  ezUuid m_Object;
  ezVariant m_NewValue;
  bool m_bEditorProperty;

  const char* GetPropertyPath() const { return m_sPropertyPath; }
  void SetPropertyPath(const char* szPath) { m_sPropertyPath = szPath; }

private:
  virtual ezStatus Do(bool bRedo) override;
  virtual ezStatus Undo(bool bFireEvents) override;
  virtual void Cleanup(CommandState state) override { }

private:
  ezDocumentObjectBase* m_pObject;
  ezVariant m_OldValue;
  ezString m_sPropertyPath;
};



