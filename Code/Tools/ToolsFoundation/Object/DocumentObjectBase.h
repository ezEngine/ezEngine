#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Uuid.h>

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectBaseProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDocumentObjectBaseProperties);

public:

  void SetName(const char* szName) { m_sName.Assign(szName); }
  const char* GetName() const { return m_sName.GetString().GetData(); }

protected:
  ezHashedString m_sName;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectBase
{
public:
  virtual ~ezDocumentObjectBase() { }

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const = 0;
  virtual const ezIReflectedTypeAccessor& GetEditorTypeAccessor() const = 0;

  const ezDocumentObjectBase* GetParent() const { return m_pParent; }
  const ezHybridArray<ezDocumentObjectBase*, 4>& GetChildren() const { return m_Children; }

  const ezUuid& GetGuid() const { return m_Guid; }

private:
  friend class ezDocumentObjectTree;
  friend class ezDocumentObjectManagerBase;

  ezUuid m_Guid;
  ezDocumentObjectBase* m_pParent;
  ezHybridArray<ezDocumentObjectBase*, 4> m_Children;
};


template<typename EditorProperties>
class ezDocumentObjectDirect : public ezDocumentObjectBase
{
public:
  ezDocumentObjectDirect(ezReflectedClass* pObjectProperties) : 
    m_pObjectProperties(pObjectProperties),
    m_ObjectPropertiesAccessor(pObjectProperties),
    m_EditorPropertiesAccessor(&m_EditorProperties)
  {
  }

  virtual ~ezDocumentObjectDirect()
  {
    EZ_ASSERT(m_pObjectProperties == nullptr, "Object has not been destroyed.");
  }

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor()       const override { return m_ObjectPropertiesAccessor; }
  virtual const ezIReflectedTypeAccessor& GetEditorTypeAccessor() const override { return m_EditorPropertiesAccessor; }

protected:
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

private:
  EditorProperties m_EditorProperties;

  ezReflectedTypeStorageAccessor m_ObjectPropertiesAccessor;
  ezReflectedTypeDirectAccessor m_EditorPropertiesAccessor;
};




