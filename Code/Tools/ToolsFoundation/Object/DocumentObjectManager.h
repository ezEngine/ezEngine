#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class ezDocumentObjectManager;
class ezDocumentBase;

class ezDocumentObjectRoot : public ezDocumentObjectBase
{
public:
  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const override { return s_Accessor; }
  virtual const ezIReflectedTypeAccessor& GetEditorTypeAccessor() const override { return s_Accessor; }

private:
  static ezEmptyProperties s_Properties;
  static ezReflectedTypeDirectAccessor s_Accessor;
};


struct ezDocumentObjectStructureEvent
{
  ezDocumentObjectStructureEvent() : m_pObject(nullptr), m_pPreviousParent(nullptr), m_pNewParent(nullptr), m_uiNewChildIndex(-1)
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

struct ezDocumentObjectPropertyEvent
{
  ezDocumentObjectPropertyEvent()
  {
    m_pObject = nullptr;
    m_bEditorProperty = false;
  }

  const ezDocumentObjectBase* m_pObject;
  ezVariant m_NewValue;
  ezString m_sPropertyPath;
  bool m_bEditorProperty;
};



class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectManager
{
public:
  ezEvent<const ezDocumentObjectStructureEvent&> m_StructureEvents;
  ezEvent<const ezDocumentObjectPropertyEvent&> m_PropertyEvents;

  ezDocumentObjectManager();
  virtual ~ezDocumentObjectManager();
  void SetDocument(const ezDocumentBase* pDocument) { m_pDocument = pDocument; }

  // Object Construction / Destruction
  ezDocumentObjectBase* CreateObject(const ezRTTI* pRtti, ezUuid guid = ezUuid());
  void DestroyObject(ezDocumentObjectBase* pObject);
  void DestroyAllObjects(ezDocumentObjectManager* pDocumentObjectManager);
  virtual void GetCreateableTypes(ezHybridArray<ezRTTI*, 32>& Types) const = 0;

  // Structure Change
  const ezDocumentObjectBase* GetRootObject() const { return &m_RootObject; }
  ezDocumentObjectBase* GetRootObject() { return &m_RootObject; }

  void AddObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pParent, ezInt32 iChildIndex = -1);
  void RemoveObject(ezDocumentObjectBase* pObject);
  void MoveObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex = -1);
  const ezDocumentObjectBase* GetObject(const ezUuid& guid) const;
  ezDocumentObjectBase* GetObject(const ezUuid& guid);
  const ezDocumentBase* GetDocument() const { return m_pDocument; }

  // Structure Change Test
  bool CanAdd(const ezRTTI* pRtti, const ezDocumentObjectBase* pParent) const;
  bool CanRemove(const ezDocumentObjectBase* pObject) const;
  bool CanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex = -1) const;

private:
  virtual ezDocumentObjectBase* InternalCreateObject(const ezRTTI* pRtti) = 0;
  virtual void InternalDestroyObject(ezDocumentObjectBase* pObject) = 0;
  virtual bool InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObjectBase* pParent) const = 0;
  virtual bool InternalCanRemove(const ezDocumentObjectBase* pObject) const = 0;
  virtual bool InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, ezInt32 iChildIndex) const = 0;

  void RecursiveAddGuids(ezDocumentObjectBase* pObject);
  void RecursiveRemoveGuids(ezDocumentObjectBase* pObject);

private:
  const ezDocumentBase* m_pDocument;
  ezDocumentObjectRoot m_RootObject;

  ezHashTable<ezUuid, const ezDocumentObjectBase*> m_GuidToObject;
};
