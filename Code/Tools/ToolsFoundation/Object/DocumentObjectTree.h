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
  ezDocumentObjectTreeEvent() : m_pObject(nullptr), m_pPreviousParent(nullptr), m_pNewParent(nullptr), m_uiNewChildIndex(-1)
  {}

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
  ezUInt32 m_uiNewChildIndex;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectTree
{
public:

  ezEvent<const ezDocumentObjectTreeEvent&> m_Events;

  ezDocumentObjectTree(const ezDocumentBase* pDocument);

  const ezDocumentObjectBase* GetRootObject() const { return &m_RootObject; }

  void AddObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pParent, ezInt32 iChildIndex = -1);

  void RemoveObject(ezDocumentObjectBase* pObject);

  void MoveObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex = -1);

  const ezDocumentObjectBase* GetObject(const ezUuid& guid) const;

  ezDocumentObjectBase* GetObject(const ezUuid& guid);

  const ezDocumentBase* GetDocument() const { return m_pDocument; }

private:
  ezDocumentObjectRoot m_RootObject;

  void RecursiveAddGuids(ezDocumentObjectBase* pObject);
  void RecursiveRemoveGuids(ezDocumentObjectBase* pObject);

  /// \todo this should be a hash map
  ezMap<ezUuid, const ezDocumentObjectBase*> m_GuidToObject;

  const ezDocumentBase* m_pDocument;
};
