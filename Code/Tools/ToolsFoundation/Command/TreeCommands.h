#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Document.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_TOOLSFOUNDATION_DLL ezAddObjectCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezAddObjectCommand, ezCommand);

public:
  ezAddObjectCommand();

public: // Properties
  void SetType(const char* szType);
  const char* GetType() const;

  const ezRTTI* m_pType;
  ezUuid m_Parent;
  ezString m_sParentProperty;
  ezVariant m_Index;
  ezUuid m_NewObjectGuid; ///< This is optional. If not filled out, a new guid is assigned automatically.

private:
  virtual bool HasReturnValues() const override { return true; }
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  ezDocumentObject* m_pObject;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


class EZ_TOOLSFOUNDATION_DLL ezPasteObjectsCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPasteObjectsCommand, ezCommand);

public:
  ezPasteObjectsCommand();

public: // Properties
  ezUuid m_Parent;
  ezString m_sGraphTextFormat;
  ezString m_sMimeType;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  struct PastedObject
  {
    ezDocumentObject* m_pObject;
    ezDocumentObject* m_pParent;
    ezString m_sParentProperty;
    ezVariant m_Index;
  };

  ezHybridArray<PastedObject, 4> m_PastedObjects;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_TOOLSFOUNDATION_DLL ezInstantiatePrefabCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInstantiatePrefabCommand, ezCommand);

public:
  ezInstantiatePrefabCommand();

public: // Properties
  ezUuid m_Parent;
  ezInt32 m_Index = -1;
  ezUuid m_CreateFromPrefab;
  ezUuid m_RemapGuid;
  ezString m_sBasePrefabGraph;
  ezString m_sObjectGraph;
  ezUuid m_CreatedRootObject;
  bool m_bAllowPickedPosition;

private:
  virtual bool HasReturnValues() const override { return true; }
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  struct PastedObject
  {
    ezDocumentObject* m_pObject;
    ezDocumentObject* m_pParent;
    ezString m_sParentProperty;
    ezVariant m_Index;
  };

  // at the moment this array always only holds a single item, the group node for the prefab
  ezHybridArray<PastedObject, 4> m_PastedObjects;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_TOOLSFOUNDATION_DLL ezUnlinkPrefabCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezUnlinkPrefabCommand, ezCommand);

public:
  ezUnlinkPrefabCommand() {}

  ezUuid m_Object;

private:
  virtual bool HasReturnValues() const override { return false; }
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  ezUuid m_OldCreateFromPrefab;
  ezUuid m_OldRemapGuid;
  ezString m_sOldGraphTextFormat;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_TOOLSFOUNDATION_DLL ezRemoveObjectCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRemoveObjectCommand, ezCommand);

public:
  ezRemoveObjectCommand();

public: // Properties
  ezUuid m_Object;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  ezDocumentObject* m_pParent;
  ezString m_sParentProperty;
  ezVariant m_Index;
  ezDocumentObject* m_pObject;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_TOOLSFOUNDATION_DLL ezMoveObjectCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMoveObjectCommand, ezCommand);

public:
  ezMoveObjectCommand();

public: // Properties
  ezUuid m_Object;
  ezUuid m_NewParent;
  ezString m_sParentProperty;
  ezVariant m_Index;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override { }

private:
  ezDocumentObject* m_pObject;
  ezDocumentObject* m_pOldParent;
  ezDocumentObject* m_pNewParent;
  ezString m_sOldParentProperty;
  ezVariant m_OldIndex;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_TOOLSFOUNDATION_DLL ezSetObjectPropertyCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSetObjectPropertyCommand, ezCommand);

public:
  ezSetObjectPropertyCommand();

public: // Properties
  ezUuid m_Object;
  ezVariant m_NewValue;
  ezVariant m_Index;
  ezString m_sProperty;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override { }

private:
  ezDocumentObject* m_pObject;
  ezVariant m_OldValue;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_TOOLSFOUNDATION_DLL ezResizeAndSetObjectPropertyCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezResizeAndSetObjectPropertyCommand, ezCommand);

public:
  ezResizeAndSetObjectPropertyCommand();

public: // Properties
  ezUuid m_Object;
  ezVariant m_NewValue;
  ezVariant m_Index;
  ezString m_sProperty;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override { return ezStatus(EZ_SUCCESS); }
  virtual void CleanupInternal(CommandState state) override { }

  ezDocumentObject* m_pObject;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_TOOLSFOUNDATION_DLL ezInsertObjectPropertyCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezInsertObjectPropertyCommand, ezCommand);

public:
  ezInsertObjectPropertyCommand();

public: // Properties
  ezUuid m_Object;
  ezVariant m_NewValue;
  ezVariant m_Index;
  ezString m_sProperty;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override { }

private:
  ezDocumentObject* m_pObject;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_TOOLSFOUNDATION_DLL ezRemoveObjectPropertyCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRemoveObjectPropertyCommand, ezCommand);

public:
  ezRemoveObjectPropertyCommand();

public: // Properties
  ezUuid m_Object;
  ezVariant m_Index;
  ezString m_sProperty;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override { }

private:
  ezDocumentObject* m_pObject;
  ezVariant m_OldValue;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_TOOLSFOUNDATION_DLL ezMoveObjectPropertyCommand : public ezCommand
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMoveObjectPropertyCommand, ezCommand);

public:
  ezMoveObjectPropertyCommand();

public: // Properties
  ezUuid m_Object;
  ezVariant m_OldIndex;
  ezVariant m_NewIndex;
  ezString m_sProperty;

private:
  virtual ezStatus DoInternal(bool bRedo) override;
  virtual ezStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override { }

private:
  ezDocumentObject* m_pObject;
};
