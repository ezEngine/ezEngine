#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Uuid.h>

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectBase
{
public:
  ezDocumentObjectBase() { m_pParent = nullptr; }
  virtual ~ezDocumentObjectBase() { }

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const = 0;
  virtual const ezIReflectedTypeAccessor& GetEditorTypeAccessor() const = 0;

  virtual ezIReflectedTypeAccessor& GetTypeAccessor();
  virtual ezIReflectedTypeAccessor& GetEditorTypeAccessor();

  const ezDocumentObjectBase* GetParent() const { return m_pParent; }
  const ezHybridArray<ezDocumentObjectBase*, 4>& GetChildren() const { return m_Children; }

  ezUInt32 GetChildIndex(ezDocumentObjectBase* pChild) const;

  const ezUuid& GetGuid() const { return m_Guid; }

private:
  friend class ezDocumentObjectTree;
  friend class ezDocumentObjectManagerBase;

  ezUuid m_Guid;
  ezDocumentObjectBase* m_pParent;
  ezHybridArray<ezDocumentObjectBase*, 4> m_Children;
};


template<typename EditorProperties, typename DirectMemberProperties>
class ezDocumentObjectDirectMember : public ezDocumentObjectBase
{
public:
  ezDocumentObjectDirectMember() : 
    m_ObjectPropertiesAccessor(&m_MemberProperties, ezGetStaticRTTI<EditorProperties>()),
    m_EditorPropertiesAccessor(&m_EditorProperties, ezGetStaticRTTI<DirectMemberProperties>())
  {
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


template<typename EditorProperties>
class ezDocumentObjectDirectPtr : public ezDocumentObjectBase
{
public:
  ezDocumentObjectDirectPtr(ezReflectedClass* pObjectProperties) : 
    m_pObjectProperties(pObjectProperties),
    m_ObjectPropertiesAccessor(pObjectProperties),
    m_EditorPropertiesAccessor(&m_EditorProperties)
  {
  }

  virtual ~ezDocumentObjectDirectPtr()
  {
    EZ_ASSERT_DEV(m_pObjectProperties == nullptr, "Object has not been destroyed.");
  }

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor()       const override { return m_ObjectPropertiesAccessor; }
  virtual const ezIReflectedTypeAccessor& GetEditorTypeAccessor() const override { return m_EditorPropertiesAccessor; }

public:
  ezReflectedClass* m_pObjectProperties;

private:
  EditorProperties m_EditorProperties;

  ezReflectedTypeDirectAccessor m_ObjectPropertiesAccessor;
  ezReflectedTypeDirectAccessor m_EditorPropertiesAccessor;
};


template<typename EditorProperties>
class ezDocumentObjectStorage : public ezDocumentObjectBase
{
public:
  ezDocumentObjectStorage(ezReflectedTypeHandle hObjectProperties) : 
    m_ObjectPropertiesAccessor(hObjectProperties),
    m_EditorPropertiesAccessor(&m_EditorProperties)
  {
  }

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor()       const override { return m_ObjectPropertiesAccessor; }
  virtual const ezIReflectedTypeAccessor& GetEditorTypeAccessor() const override { return m_EditorPropertiesAccessor; }

public:
  EditorProperties m_EditorProperties;

private:
  ezReflectedTypeStorageAccessor m_ObjectPropertiesAccessor;
  ezReflectedTypeDirectAccessor m_EditorPropertiesAccessor;
};




