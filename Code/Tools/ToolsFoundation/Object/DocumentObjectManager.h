#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class ezDocumentObjectManager;
class ezDocumentBase;

class ezDocumentRoot : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentRoot);

  ezHybridArray<ezReflectedClass*, 8> m_RootObjects;
};

class ezDocumentObjectRoot : public ezDocumentObject
{
public:
  ezDocumentObjectRoot() : ezDocumentObject(ezDocumentRoot::GetStaticRTTI())
  {
    m_Guid = ezUuid::StableUuidForString("DocumentRoot");
  }

public:
  virtual void InsertSubObject(ezDocumentObjectBase* pObject, const char* szProperty, const ezVariant& index) override;
  virtual void RemoveSubObject(ezDocumentObjectBase* pObject) override;
};


struct ezDocumentObjectStructureEvent
{
  ezDocumentObjectStructureEvent() : m_pObject(nullptr), m_pPreviousParent(nullptr), m_pNewParent(nullptr)
  {}

  enum class Type
  {
    BeforeObjectAdded,
    AfterObjectAdded,
    BeforeObjectRemoved,
    AfterObjectRemoved,
    BeforeObjectMoved,
    AfterObjectMoved,
    AfterObjectMoved2,
  };

  Type m_EventType;
  const ezDocumentObjectBase* m_pObject;
  const ezDocumentObjectBase* m_pPreviousParent;
  const ezDocumentObjectBase* m_pNewParent;
  ezString m_sParentProperty;
  ezVariant m_PropertyIndex;
};

struct ezDocumentObjectPropertyEvent
{
  ezDocumentObjectPropertyEvent()
  {
    m_pObject = nullptr;
  }

  enum class Type
  {
    PropertySet,
    PropertyInserted,
    PropertyRemoved,
    PropertyMoved,
  };

  Type m_EventType;
  const ezDocumentObjectBase* m_pObject;
  ezVariant m_OldValue;
  ezVariant m_NewValue;
  ezString m_sPropertyPath;
  ezVariant m_OldIndex;
  ezVariant m_NewIndex;
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
  // holds object data
  ezDocumentObjectBase* CreateObject(const ezRTTI* pRtti, ezUuid guid = ezUuid());

  void DestroyObject(ezDocumentObjectBase* pObject);
  void DestroyAllObjects();
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const = 0;

  // Structure Change
  const ezDocumentObjectBase* GetRootObject() const { return &m_RootObject; }
  ezDocumentObjectBase* GetRootObject() { return &m_RootObject; }

  void AddObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pParent, const char* szParentProperty, ezVariant index);
  void RemoveObject(ezDocumentObjectBase* pObject);
  void MoveObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pNewParent, const char* szParentProperty, ezVariant index);
  
  
  const ezDocumentObjectBase* GetObject(const ezUuid& guid) const;
  ezDocumentObjectBase* GetObject(const ezUuid& guid);
  const ezDocumentBase* GetDocument() const { return m_pDocument; }

  // Structure Change Test
  bool CanAdd(const ezRTTI* pRtti, const ezDocumentObjectBase* pParent, const char* szParentProperty, const ezVariant& index) const;
  bool CanRemove(const ezDocumentObjectBase* pObject) const;
  bool CanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, const char* szParentProperty, const ezVariant& index) const;

private:
  virtual ezDocumentObjectBase* InternalCreateObject(const ezRTTI* pRtti) = 0;
  virtual void InternalDestroyObject(ezDocumentObjectBase* pObject) { EZ_DEFAULT_DELETE(pObject); }
  virtual bool InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObjectBase* pParent, const char* szParentProperty, const ezVariant& index) const { return true; };
  virtual bool InternalCanRemove(const ezDocumentObjectBase* pObject) const { return true; };
  virtual bool InternalCanMove(const ezDocumentObjectBase* pObject, const ezDocumentObjectBase* pNewParent, const char* szParentProperty, const ezVariant& index) const { return true; };

  void RecursiveAddGuids(ezDocumentObjectBase* pObject);
  void RecursiveRemoveGuids(ezDocumentObjectBase* pObject);

private:
  const ezDocumentBase* m_pDocument;
  ezDocumentObjectRoot m_RootObject;

  ezHashTable<ezUuid, const ezDocumentObjectBase*> m_GuidToObject;
};
