#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <Foundation/Containers/Map.h>

class ezDocumentBase;

class ezEmptyProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEmptyProperties);
};

class ezDocumentObjectRoot : public ezDocumentObjectBase
{
public:
  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const override { return s_Accessor; }
  virtual const ezIReflectedTypeAccessor& GetEditorTypeAccessor() const override { return s_Accessor; }

private:
  static ezEmptyProperties s_Properties;
  static ezReflectedTypeDirectAccessor s_Accessor;
};

struct ezDocumentObjectTreeEvent
{
  enum class Type
  {
    BeforeObjectAdded,
    AfterObjectAdded,
    BeforeObjectRemoved,
    AfterObjectRemoved,
    BeforeObjectMoved,
    AfterObjectMoved,
  };

  Type m_EventType;
  const ezDocumentObjectBase* m_pObject;
  const ezDocumentObjectBase* m_pPreviousParent;
  const ezDocumentObjectBase* m_pNewParent;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectTree
{
public:

  ezEvent<const ezDocumentObjectTreeEvent&> m_Events;

  ezDocumentObjectTree(const ezDocumentBase* pDocument);

  const ezDocumentObjectBase* GetRootObject() const { return &m_RootObject; }

  void AddObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pParent);

  void RemoveObject(ezDocumentObjectBase* pObject);

  void MoveObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pNewParent);

  const ezDocumentObjectBase* GetObject(const ezUuid& guid) const;

  const ezDocumentBase* GetDocument() const { return m_pDocument; }

private:
  ezDocumentObjectRoot m_RootObject;

  void RecursiveAddGuids(ezDocumentObjectBase* pObject);
  void RecursiveRemoveGuids(ezDocumentObjectBase* pObject);

  /// \todo this should be a hash map
  ezMap<ezUuid, const ezDocumentObjectBase*> m_GuidToObject;

  const ezDocumentBase* m_pDocument;
};
