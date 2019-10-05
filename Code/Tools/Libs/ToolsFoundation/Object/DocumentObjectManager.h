#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class ezDocumentObjectManager;
class ezDocument;

// Prevent conflicts with windows.h
#ifdef GetObject
#undef GetObject
#endif

class EZ_TOOLSFOUNDATION_DLL ezDocumentRoot : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentRoot, ezReflectedClass);

  ezHybridArray<ezReflectedClass*, 1> m_RootObjects;
  ezHybridArray<ezReflectedClass*, 1> m_TempObjects;
};

class ezDocumentRootObject : public ezDocumentStorageObject
{
public:
  ezDocumentRootObject(const ezRTTI* pRootType) : ezDocumentStorageObject(pRootType)
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

  const ezAbstractProperty* GetProperty() const;
  ezVariant getInsertIndex() const;
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
  const ezDocument* m_pDocument = nullptr;
  const ezDocumentObject* m_pObject = nullptr;
  const ezDocumentObject* m_pPreviousParent = nullptr;
  const ezDocumentObject* m_pNewParent = nullptr;
  ezString m_sParentProperty;
  ezVariant m_OldPropertyIndex;
  ezVariant m_NewPropertyIndex;
};

struct ezDocumentObjectPropertyEvent
{
  ezDocumentObjectPropertyEvent()
  {
    m_pObject = nullptr;
  }
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
  mutable ezEvent<const ezDocumentObjectStructureEvent&> m_StructureEvents;
  mutable ezEvent<const ezDocumentObjectPropertyEvent&> m_PropertyEvents;
  ezEvent<const ezDocumentObjectEvent&> m_ObjectEvents;

  ezDocumentObjectManager(const ezRTTI* pRootType = ezDocumentRoot::GetStaticRTTI());
  virtual ~ezDocumentObjectManager();
  void SetDocument(const ezDocument* pDocument) { m_pDocument = pDocument; }

  // Object Construction / Destruction
  // holds object data
  ezDocumentObject* CreateObject(const ezRTTI* pRtti, ezUuid guid = ezUuid());

  void DestroyObject(ezDocumentObject* pObject);
  virtual void DestroyAllObjects();
  virtual void GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const {};

  /// \brief Allows to annotate types with a category (group), such that things like creator menus can use this to present the types in a more user friendly way
  virtual const char* GetTypeCategory(const ezRTTI* pRtti) const { return nullptr; }
  void PatchEmbeddedClassObjects(const ezDocumentObject* pObject) const;

  const ezDocumentObject* GetRootObject() const { return &m_RootObject; }
  ezDocumentObject* GetRootObject() { return &m_RootObject; }
  const ezDocumentObject* GetObject(const ezUuid& guid) const;
  ezDocumentObject* GetObject(const ezUuid& guid);
  const ezDocument* GetDocument() const { return m_pDocument; }

  // Property Change
  ezStatus SetValue(ezDocumentObject* pObject, const char* szProperty, const ezVariant& newValue, ezVariant index = ezVariant());
  ezStatus InsertValue(ezDocumentObject* pObject, const char* szProperty, const ezVariant& newValue, ezVariant index = ezVariant());
  ezStatus RemoveValue(ezDocumentObject* pObject, const char* szProperty, ezVariant index = ezVariant());
  ezStatus MoveValue(ezDocumentObject* pObject, const char* szProperty, const ezVariant& oldIndex, const ezVariant& newIndex);

  // Structure Change
  void AddObject(ezDocumentObject* pObject, ezDocumentObject* pParent, const char* szParentProperty, ezVariant index);
  void RemoveObject(ezDocumentObject* pObject);
  void MoveObject(ezDocumentObject* pObject, ezDocumentObject* pNewParent, const char* szParentProperty, ezVariant index);

  // Structure Change Test
  ezStatus CanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, const char* szParentProperty, const ezVariant& index) const;
  ezStatus CanRemove(const ezDocumentObject* pObject) const;
  ezStatus CanMove(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const char* szParentProperty, const ezVariant& index) const;
  ezStatus CanSelect(const ezDocumentObject* pObject) const;

  bool IsUnderRootProperty(const char* szRootProperty, const ezDocumentObject* pObject) const;
  bool IsUnderRootProperty(const char* szRootProperty, const ezDocumentObject* pParent, const char* szParentProperty) const;

private:
  virtual ezDocumentObject* InternalCreateObject(const ezRTTI* pRtti) { return EZ_DEFAULT_NEW(ezDocumentStorageObject, pRtti); }
  virtual void InternalDestroyObject(ezDocumentObject* pObject) { EZ_DEFAULT_DELETE(pObject); }

  void InternalAddObject(ezDocumentObject* pObject, ezDocumentObject* pParent, const char* szParentProperty, ezVariant index);
  void InternalRemoveObject(ezDocumentObject* pObject);
  void InternalMoveObject(ezDocumentObject* pNewParent, ezDocumentObject* pObject, const char* szParentProperty, ezVariant index);

  virtual ezStatus InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, const char* szParentProperty, const ezVariant& index) const { return ezStatus(EZ_SUCCESS); };
  virtual ezStatus InternalCanRemove(const ezDocumentObject* pObject) const { return ezStatus(EZ_SUCCESS); };
  virtual ezStatus InternalCanMove(const ezDocumentObject* pObject, const ezDocumentObject* pNewParent, const char* szParentProperty, const ezVariant& index) const { return ezStatus(EZ_SUCCESS); };
  virtual ezStatus InternalCanSelect(const ezDocumentObject* pObject) const { return ezStatus(EZ_SUCCESS); };

  void RecursiveAddGuids(ezDocumentObject* pObject);
  void RecursiveRemoveGuids(ezDocumentObject* pObject);
  void PatchEmbeddedClassObjectsInternal(ezDocumentObject* pObject, const ezRTTI* pType, bool addToDoc);

private:
  friend class ezObjectAccessorBase;

  const ezDocument* m_pDocument;
  ezDocumentRootObject m_RootObject;

  ezHashTable<ezUuid, const ezDocumentObject*> m_GuidToObject;
};
