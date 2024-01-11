#pragma once

#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class ezDocumentObjectManager;
class ezDocument;

// Prevent conflicts with windows.h
#ifdef GetObject
#  undef GetObject
#endif

/// \brief Standard root object for most documents.
/// m_RootObjects stores what is in the document and m_TempObjects stores transient data used during editing which is not part of the document.
class EZ_TOOLSFOUNDATION_DLL ezDocumentRoot : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentRoot, ezReflectedClass);

  ezHybridArray<ezReflectedClass*, 1> m_RootObjects;
  ezHybridArray<ezReflectedClass*, 1> m_TempObjects;
};

/// \brief Implementation detail of ezDocumentObjectManager.
class ezDocumentRootObject : public ezDocumentStorageObject
{
public:
  ezDocumentRootObject(const ezRTTI* pRootType)
    : ezDocumentStorageObject(pRootType)
  {
    m_Guid = ezUuid::MakeStableUuidFromString("DocumentRoot");
  }

public:
  virtual void InsertSubObject(ezDocumentObject* pObject, ezStringView sProperty, const ezVariant& index) override;
  virtual void RemoveSubObject(ezDocumentObject* pObject) override;
};

/// \brief Used by ezDocumentObjectManager::m_StructureEvents.
struct ezDocumentObjectStructureEvent
{
  ezDocumentObjectStructureEvent()

    = default;

  const ezAbstractProperty* GetProperty() const;
  ezVariant getInsertIndex() const;
  enum class Type
  {
    BeforeReset,
    AfterReset,
    BeforeObjectAdded,
    AfterObjectAdded,
    BeforeObjectRemoved,
    AfterObjectRemoved,
    BeforeObjectMoved,
    AfterObjectMoved,
    AfterObjectMoved2,
  };

  Type m_EventType;
  const ezDocument* m_pDocument = nullptr;
  const ezDocumentObject* m_pObject = nullptr;
  const ezDocumentObject* m_pPreviousParent = nullptr;
  const ezDocumentObject* m_pNewParent = nullptr;
  ezString m_sParentProperty;
  ezVariant m_OldPropertyIndex;
  ezVariant m_NewPropertyIndex;
};

/// \brief Used by ezDocumentObjectManager::m_PropertyEvents.
struct ezDocumentObjectPropertyEvent
{
  ezDocumentObjectPropertyEvent() { m_pObject = nullptr; }
  ezVariant getInsertIndex() const;

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
  ezString m_sProperty;
  ezVariant m_OldIndex;
  ezVariant m_NewIndex;
};

/// \brief Used by ezDocumentObjectManager::m_ObjectEvents.
struct ezDocumentObjectEvent
{
  ezDocumentObjectEvent() { m_pObject = nullptr; }

  enum class Type
  {
    BeforeObjectDestroyed,
    AfterObjectCreated,
    Invalid
  };

  Type m_EventType = Type::Invalid;
  const ezDocumentObject* m_pObject;
};

/// \brief Represents to content of a document. Every document has exactly one root object under which all objects need to be parented. The default root object is ezDocumentRoot.
class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectManager
{
public:
  // \brief Storage for the object manager so it can be swapped when using multiple sub documents.
  class Storage : public ezRefCounted
  {
  public:
    Storage(const ezRTTI* pRootType);

    ezDocument* m_pDocument = nullptr;
    ezDocumentRootObject m_RootObject;

    ezHashTable<ezUuid, const ezDocumentObject*> m_GuidToObject;

    mutable ezCopyOnBroadcastEvent<const ezDocumentObjectStructureEvent&> m_StructureEvents;
    mutable ezCopyOnBroadcastEvent<const ezDocumentObjectPropertyEvent&> m_PropertyEvents;
    ezEvent<const ezDocumentObjectEvent&> m_ObjectEvents;
  };

public:
  mutable ezCopyOnBroadcastEvent<const ezDocumentObjectStructureEvent&> m_StructureEvents;
  mutable ezCopyOnBroadcastEvent<const ezDocumentObjectPropertyEvent&> m_PropertyEvents;
  ezEvent<const ezDocumentObjectEvent&> m_ObjectEvents;

  ezDocumentObjectManager(const ezRTTI* pRootType = ezDocumentRoot::GetStaticRTTI());
  virtual ~ezDocumentObjectManager();
  void SetDocument(ezDocument* pDocument) { m_pObjectStorage->m_pDocument = pDocument; }

  // Object Construction / Destruction
  // holds object data
  ezDocumentObject* CreateObject(const ezRTTI* pRtti, ezUuid guid = ezUuid());

  void DestroyObject(ezDocumentObject* pObject);
  virtual void DestroyAllObjects();
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& ref_types) const {};

  void PatchEmbeddedClassObjects(const ezDocumentObject* pObject) const;

  const ezDocumentObject* GetRootObject() const { return &m_pObjectStorage->m_RootObject; }
  ezDocumentObject* GetRootObject() { return &m_pObjectStorage->m_RootObject; }
  const ezDocumentObject* GetObject(const ezUuid& guid) const;
  ezDocumentObject* GetObject(const ezUuid& guid);
  const ezDocument* GetDocument() const { return m_pObjectStorage->m_pDocument; }
  ezDocument* GetDocument() { return m_pObjectStorage->m_pDocument; }

  // Property Change
  ezStatus SetValue(ezDocumentObject* pObject, ezStringView sProperty, const ezVariant& newValue, ezVariant index = ezVariant());
  ezStatus InsertValue(ezDocumentObject* pObject, ezStringView sProperty, const ezVariant& newValue, ezVariant index = ezVariant());
  ezStatus RemoveValue(ezDocumentObject* pObject, ezStringView sProperty, ezVariant index = ezVariant());
  ezStatus MoveValue(ezDocumentObject* pObject, ezStringView sProperty, const ezVariant& oldIndex, const ezVariant& newIndex);

  // Structure Change
  void AddObject(ezDocumentObject* pObject, ezDocumentObject* pParent, ezStringView sParentProperty, ezVariant index);
  void RemoveObject(ezDocumentObject* pObject);
  void MoveObject(ezDocumentObject* pObject, ezDocumentObject* pNewParent, ezStringView sParentProperty, ezVariant index);

  // Structure Change Test
  ezStatus CanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, ezStringView sParentProperty, const ezVariant& index) const;
  ezStatus CanRemove(const ezDocumentObject* pObject) const;
  ezStatus CanMove(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, ezStringView sParentProperty, const ezVariant& index) const;
  ezStatus CanSelect(const ezDocumentObject* pObject) const;

  bool IsUnderRootProperty(ezStringView sRootProperty, const ezDocumentObject* pObject) const;
  bool IsUnderRootProperty(ezStringView sRootProperty, const ezDocumentObject* pParent, ezStringView sParentProperty) const;
  bool IsTemporary(const ezDocumentObject* pObject) const;
  bool IsTemporary(const ezDocumentObject* pParent, ezStringView sParentProperty) const;

  ezSharedPtr<ezDocumentObjectManager::Storage> SwapStorage(ezSharedPtr<ezDocumentObjectManager::Storage> pNewStorage);
  ezSharedPtr<ezDocumentObjectManager::Storage> GetStorage() { return m_pObjectStorage; }

private:
  virtual ezDocumentObject* InternalCreateObject(const ezRTTI* pRtti) { return EZ_DEFAULT_NEW(ezDocumentStorageObject, pRtti); }
  virtual void InternalDestroyObject(ezDocumentObject* pObject) { EZ_DEFAULT_DELETE(pObject); }

  void InternalAddObject(ezDocumentObject* pObject, ezDocumentObject* pParent, ezStringView sParentProperty, ezVariant index);
  void InternalRemoveObject(ezDocumentObject* pObject);
  void InternalMoveObject(ezDocumentObject* pNewParent, ezDocumentObject* pObject, ezStringView sParentProperty, ezVariant index);

  virtual ezStatus InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, ezStringView sParentProperty, const ezVariant& index) const
  {
    return ezStatus(EZ_SUCCESS);
  };
  virtual ezStatus InternalCanRemove(const ezDocumentObject* pObject) const { return ezStatus(EZ_SUCCESS); };
  virtual ezStatus InternalCanMove(
    const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, ezStringView sParentProperty, const ezVariant& index) const
  {
    return ezStatus(EZ_SUCCESS);
  };
  virtual ezStatus InternalCanSelect(const ezDocumentObject* pObject) const { return ezStatus(EZ_SUCCESS); };

  void RecursiveAddGuids(ezDocumentObject* pObject);
  void RecursiveRemoveGuids(ezDocumentObject* pObject);
  void PatchEmbeddedClassObjectsInternal(ezDocumentObject* pObject, const ezRTTI* pType, bool addToDoc);

private:
  friend class ezObjectAccessorBase;

  ezSharedPtr<ezDocumentObjectManager::Storage> m_pObjectStorage;

  ezCopyOnBroadcastEvent<const ezDocumentObjectStructureEvent&>::Unsubscriber m_StructureEventsUnsubscriber;
  ezCopyOnBroadcastEvent<const ezDocumentObjectPropertyEvent&>::Unsubscriber m_PropertyEventsUnsubscriber;
  ezEvent<const ezDocumentObjectEvent&>::Unsubscriber m_ObjectEventsUnsubscriber;
};
