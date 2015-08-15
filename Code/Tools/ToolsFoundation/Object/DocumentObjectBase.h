#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeSubObjectAccessor.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Uuid.h>

class ezDocumentObjectManager;

struct ezDocumentObjectType
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    Root,
    Object,
    ArrayElement,
    SetElement,
    SubObject,
    Default = Object
  };
};


class ezEmptyProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEmptyProperties);
};


class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectBase
{
public:
  ezDocumentObjectBase()
    : m_pDocumentObjectManager(nullptr)
    , m_pParent(nullptr)
    , m_bEditorProperty(false)
  {
  }
  virtual ~ezDocumentObjectBase() = 0 { }

  // Accessors
  const ezUuid& GetGuid() const { return m_Guid; }
  ezEnum<ezDocumentObjectType> GetObjectType() const { return m_ObjectType; }
  const ezDocumentObjectManager* GetDocumentObjectManager() const { return m_pDocumentObjectManager; }
  bool IsEditorProperty() const { return m_bEditorProperty; }

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const = 0;
  virtual const ezIReflectedTypeAccessor& GetEditorTypeAccessor() const = 0;
  ezIReflectedTypeAccessor& GetTypeAccessor();
  ezIReflectedTypeAccessor& GetEditorTypeAccessor();

  const ezIReflectedTypeAccessor& GetParentAccessor() const;

  // Ownership
  const ezDocumentObjectBase* GetParent() const { return m_pParent; }

  virtual void InsertSubObject(ezDocumentObjectBase* pObject, const char* szProperty, const ezVariant& index, bool bEditorProperty);
  virtual void RemoveSubObject(ezDocumentObjectBase* pObject);

  // Helper
  void ComputeObjectHash(ezUInt64& uiHash) const;
  const ezHybridArray<ezDocumentObjectBase*, 8>& GetChildren() const { return m_Children; }
  const char* GetParentProperty() const { return m_sParentProperty; }
  ezVariant GetPropertyIndex() const;

private:
  friend class ezDocumentObjectManager;
  void HashPropertiesRecursive(const ezIReflectedTypeAccessor& acc, ezUInt64& uiHash, const ezRTTI* pType, ezPropertyPath& path) const;
  ezUInt32 GetChildIndex(ezDocumentObjectBase* pChild) const;

protected:
  ezUuid m_Guid;
  ezEnum<ezDocumentObjectType> m_ObjectType;
  const ezDocumentObjectManager* m_pDocumentObjectManager;

  ezDocumentObjectBase* m_pParent;
  ezHybridArray<ezDocumentObjectBase*, 8> m_Children;

  // Sub object data
  ezString m_sParentProperty;
  bool m_bEditorProperty;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentObject : public ezDocumentObjectBase
{
public:
  ezDocumentObject(const ezRTTI* pEditor, const ezRTTI* pObject)
    : ezDocumentObjectBase()
    , m_EditorPropertiesAccessor(pEditor, this)
    , m_ObjectPropertiesAccessor(pObject, this)
  {
    m_ObjectType = ezDocumentObjectType::Object;
  }
  virtual ~ezDocumentObject() { }

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const override { return m_ObjectPropertiesAccessor; }
  virtual const ezIReflectedTypeAccessor& GetEditorTypeAccessor() const override { return m_EditorPropertiesAccessor; }

protected:
  ezReflectedTypeStorageAccessor m_EditorPropertiesAccessor;
  ezReflectedTypeStorageAccessor m_ObjectPropertiesAccessor;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentSubElementObject : public ezDocumentObjectBase
{
public:
  ezDocumentSubElementObject(const ezRTTI* pRtti, ezEnum<ezDocumentObjectType> type)
    : ezDocumentObjectBase()
    , m_Accessor(pRtti, this)
  {
    m_ObjectType = type;
  }

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const override { EZ_ASSERT_DEV(!m_bEditorProperty, ""); return m_Accessor; }
  virtual const ezIReflectedTypeAccessor& GetEditorTypeAccessor() const override { EZ_ASSERT_DEV(m_bEditorProperty, ""); return m_Accessor; }

public:
  ezReflectedTypeStorageAccessor m_Accessor;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentSubObject : public ezDocumentObjectBase
{
public:
  ezDocumentSubObject(const ezRTTI* pRtti);
  void SetObject(ezDocumentObjectBase* pOwnerObject, const ezPropertyPath& subPath, bool bEditorProperty);

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const override { EZ_ASSERT_DEV(!m_bEditorProperty, ""); return m_Accessor; }
  virtual const ezIReflectedTypeAccessor& GetEditorTypeAccessor() const override { EZ_ASSERT_DEV(m_bEditorProperty, ""); return m_Accessor; }

public:
  ezReflectedTypeSubObjectAccessor m_Accessor;
  ezPropertyPath m_SubPath;
};


template<typename EditorProperties, typename DirectMemberProperties>
class ezDocumentObjectDirectMember : public ezDocumentObjectBase
{
public:
  ezDocumentObjectDirectMember() :
    m_ObjectPropertiesAccessor(&m_MemberProperties, ezGetStaticRTTI<DirectMemberProperties>(), this),
    m_EditorPropertiesAccessor(&m_EditorProperties, ezGetStaticRTTI<EditorProperties>(), this)
  {
    m_ObjectType = ezDocumentObjectType::Object;
  }

  virtual ~ezDocumentObjectDirectMember()
  {
  }

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor()       const override { return m_ObjectPropertiesAccessor; }
  virtual const ezIReflectedTypeAccessor& GetEditorTypeAccessor() const override { return m_EditorPropertiesAccessor; }

public:
  DirectMemberProperties m_MemberProperties;
  EditorProperties m_EditorProperties;

private:
  ezReflectedTypeDirectAccessor m_ObjectPropertiesAccessor;
  ezReflectedTypeDirectAccessor m_EditorPropertiesAccessor;
};