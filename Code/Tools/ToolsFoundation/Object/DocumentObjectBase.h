#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeSubObjectAccessor.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Uuid.h>

class ezDocumentObjectManager;

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectBase
{
public:
  ezDocumentObjectBase()
    : m_pDocumentObjectManager(nullptr)
    , m_pParent(nullptr)
  {
  }
  virtual ~ezDocumentObjectBase() { }

  // Accessors
  const ezUuid& GetGuid() const { return m_Guid; }
  
  const ezDocumentObjectManager* GetDocumentObjectManager() const { return m_pDocumentObjectManager; }
  ezDocumentObjectManager* GetDocumentObjectManager() { return m_pDocumentObjectManager; }

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const = 0;
  ezIReflectedTypeAccessor& GetTypeAccessor();

  // Ownership
  const ezDocumentObjectBase* GetParent() const { return m_pParent; }

  virtual void InsertSubObject(ezDocumentObjectBase* pObject, const char* szProperty, const ezVariant& index);
  virtual void RemoveSubObject(ezDocumentObjectBase* pObject);

  // Helper
  void ComputeObjectHash(ezUInt64& uiHash) const;
  const ezHybridArray<ezDocumentObjectBase*, 8>& GetChildren() const { return m_Children; }
  const char* GetParentProperty() const { return m_sParentProperty; }
  ezVariant GetPropertyIndex() const;
  bool IsOnHeap() const;

private:
  friend class ezDocumentObjectManager;
  void HashPropertiesRecursive(const ezIReflectedTypeAccessor& acc, ezUInt64& uiHash, const ezRTTI* pType, ezPropertyPath& path) const;
  ezUInt32 GetChildIndex(ezDocumentObjectBase* pChild) const;

protected:
  ezUuid m_Guid;
  ezDocumentObjectManager* m_pDocumentObjectManager;

  ezDocumentObjectBase* m_pParent;
  ezHybridArray<ezDocumentObjectBase*, 8> m_Children;

  // Sub object data
  ezString m_sParentProperty;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentObject : public ezDocumentObjectBase
{
public:
  ezDocumentObject(const ezRTTI* pObject)
    : ezDocumentObjectBase()
    , m_ObjectPropertiesAccessor(pObject, this)
  {
  }

  virtual ~ezDocumentObject() { }

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const override { return m_ObjectPropertiesAccessor; }

protected:
  ezReflectedTypeStorageAccessor m_ObjectPropertiesAccessor;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentSubObject : public ezDocumentObjectBase
{
public:
  ezDocumentSubObject(const ezRTTI* pRtti);
  void SetObject(ezDocumentObjectBase* pOwnerObject, const ezPropertyPath& subPath, ezUuid guid = ezUuid());

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const override { return m_Accessor; }

public:
  ezReflectedTypeSubObjectAccessor m_Accessor;
  ezPropertyPath m_SubPath;
};


template<typename DirectMemberProperties>
class ezDocumentObjectDirectMember : public ezDocumentObjectBase
{
public:
  ezDocumentObjectDirectMember() :
    m_ObjectPropertiesAccessor(&m_MemberProperties, ezGetStaticRTTI<DirectMemberProperties>(), this)
  {
  }

  virtual ~ezDocumentObjectDirectMember()
  {
  }

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor()       const override { return m_ObjectPropertiesAccessor; }

public:
  DirectMemberProperties m_MemberProperties;

private:
  ezReflectedTypeDirectAccessor m_ObjectPropertiesAccessor;
};