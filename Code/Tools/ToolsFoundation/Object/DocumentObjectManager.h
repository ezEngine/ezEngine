#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Basics/Status.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class ezDocumentObjectManager;
class ezDocument;

class ezDocumentRoot : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentRoot, ezReflectedClass);

  ezHybridArray<ezReflectedClass*, 8> m_RootObjects;
};

class ezDocumentRootObject : public ezDocumentStorageObject
{
public:
  ezDocumentRootObject() : ezDocumentStorageObject(ezDocumentRoot::GetStaticRTTI())
  {
    m_Guid = ezUuid::StableUuidForString("DocumentRoot");
  }

public:
  virtual void InsertSubObject(ezDocumentObject* pObject, const char* szProperty, const ezVariant& index) override;
  virtual void RemoveSubObject(ezDocumentObject* pObject) override;
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
  const ezDocumentObject* m_pObject;
  const ezDocumentObject* m_pPreviousParent;
  const ezDocumentObject* m_pNewParent;
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
  const ezDocumentObject* m_pObject;
  ezVariant m_OldValue;
  ezVariant m_NewValue;
  ezString m_sPropertyPath;
  ezVariant m_OldIndex;
  ezVariant m_NewIndex;
};

struct ezDocumentObjectEvent
{
  ezDocumentObjectEvent()
  {
    m_pObject = nullptr;
  }

  enum class Type
  {
    BeforeObjectDestroyed,
    AfterObjectCreated,
  };

  Type m_EventType;
  const ezDocumentObject* m_pObject;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectManager
{
public:
  ezEvent<const ezDocumentObjectStructureEvent&> m_StructureEvents;
  ezEvent<const ezDocumentObjectPropertyEvent&> m_PropertyEvents;
  ezEvent<const ezDocumentObjectEvent&> m_ObjectEvents;

  ezDocumentObjectManager();
  virtual ~ezDocumentObjectManager();
  void SetDocument(const ezDocument* pDocument) { m_pDocument = pDocument; }

  // Object Construction / Destruction
  // holds object data
  ezDocumentObject* CreateObject(const ezRTTI* pRtti, ezUuid guid = ezUuid());

  void DestroyObject(ezDocumentObject* pObject);
  virtual void DestroyAllObjects();
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const = 0;

  // Structure Change
  const ezDocumentObject* GetRootObject() const { return &m_RootObject; }
  ezDocumentObject* GetRootObject() { return &m_RootObject; }

  void AddObject(ezDocumentObject* pObject, ezDocumentObject* pParent, const char* szParentProperty, ezVariant index);
  void RemoveObject(ezDocumentObject* pObject);
  void MoveObject(ezDocumentObject* pObject, ezDocumentObject* pNewParent, const char* szParentProperty, ezVariant index);
  
  
  const ezDocumentObject* GetObject(const ezUuid& guid) const;
  ezDocumentObject* GetObject(const ezUuid& guid);
  const ezDocument* GetDocument() const { return m_pDocument; }

  // Structure Change Test
  ezStatus CanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, const char* szParentProperty, const ezVariant& index) const;
  ezStatus CanRemove(const ezDocumentObject* pObject) const;
  ezStatus CanMove(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const char* szParentProperty, const ezVariant& index) const;

private:
  virtual ezDocumentObject* InternalCreateObject(const ezRTTI* pRtti) { return EZ_DEFAULT_NEW(ezDocumentStorageObject, pRtti); }
  virtual void InternalDestroyObject(ezDocumentObject* pObject) { EZ_DEFAULT_DELETE(pObject); }
  virtual ezStatus InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, const char* szParentProperty, const ezVariant& index) const { return ezStatus(EZ_SUCCESS); };
  virtual ezStatus InternalCanRemove(const ezDocumentObject* pObject) const { return ezStatus(EZ_SUCCESS); };
  virtual ezStatus InternalCanMove(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const char* szParentProperty, const ezVariant& index) const { return ezStatus(EZ_SUCCESS); };

  void RecursiveAddGuids(ezDocumentObject* pObject);
  void RecursiveRemoveGuids(ezDocumentObject* pObject);

private:
  const ezDocument* m_pDocument;
  ezDocumentRootObject m_RootObject;

  ezHashTable<ezUuid, const ezDocumentObject*> m_GuidToObject;
};
