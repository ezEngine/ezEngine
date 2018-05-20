#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Uuid.h>

class ezDocumentObjectManager;

class EZ_TOOLSFOUNDATION_DLL ezDocumentObject
{
public:
  ezDocumentObject()
    : m_pDocumentObjectManager(nullptr)
    , m_pParent(nullptr)
  {
  }
  virtual ~ezDocumentObject() { }

  // Accessors
  const ezUuid& GetGuid() const { return m_Guid; }
  const ezRTTI* GetType() const { return GetTypeAccessor().GetType(); }

  const ezDocumentObjectManager* GetDocumentObjectManager() const { return m_pDocumentObjectManager; }
  ezDocumentObjectManager* GetDocumentObjectManager() { return m_pDocumentObjectManager; }

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const = 0;
  ezIReflectedTypeAccessor& GetTypeAccessor();

  // Ownership
  const ezDocumentObject* GetParent() const { return m_pParent; }

  virtual void InsertSubObject(ezDocumentObject* pObject, const char* szProperty, const ezVariant& index);
  virtual void RemoveSubObject(ezDocumentObject* pObject);

  // Helper
  void ComputeObjectHash(ezUInt64& uiHash) const;
  const ezHybridArray<ezDocumentObject*, 8>& GetChildren() const { return m_Children; }
  ezDocumentObject* GetChild(const ezUuid& guid);
  const ezDocumentObject* GetChild(const ezUuid& guid) const;
  const char* GetParentProperty() const { return m_sParentProperty; }
  ezAbstractProperty* GetParentPropertyType() const;
  ezVariant GetPropertyIndex() const;
  bool IsOnHeap() const;
  ezUInt32 GetChildIndex(const ezDocumentObject* pChild) const;

private:
  friend class ezDocumentObjectManager;
  void HashPropertiesRecursive(const ezIReflectedTypeAccessor& acc, ezUInt64& uiHash, const ezRTTI* pType) const;

protected:
  ezUuid m_Guid;
  ezDocumentObjectManager* m_pDocumentObjectManager;

  ezDocumentObject* m_pParent;
  ezHybridArray<ezDocumentObject*, 8> m_Children;

  // Sub object data
  ezString m_sParentProperty;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentStorageObject : public ezDocumentObject
{
public:
  ezDocumentStorageObject(const ezRTTI* pType)
    : ezDocumentObject()
    , m_ObjectPropertiesAccessor(pType, this)
  {
  }

  virtual ~ezDocumentStorageObject() { }

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const override { return m_ObjectPropertiesAccessor; }

protected:
  ezReflectedTypeStorageAccessor m_ObjectPropertiesAccessor;
};
