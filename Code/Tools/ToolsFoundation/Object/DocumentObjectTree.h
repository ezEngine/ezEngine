#pragma once

#include <ToolsFoundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>
#include <Foundation/Containers/Map.h>

class ezEmptyProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEmptyProperties);
};

class ezDocumentObjectRoot : public ezDocumentObjectBase
{
public:
  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const override { return s_Accessor; }
  virtual const ezIReflectedTypeAccessor& GetEditorTypeAccessor() const override { return s_Accessor; }

private:
  static ezEmptyProperties s_Properties;
  static ezReflectedTypeDirectAccessor s_Accessor;
};

class EZ_TOOLSFOUNDATION_DLL ezDocumentObjectTree
{
public:

  struct Event
  {
    enum class Type
    {
      ObjectAdded,
      ObjectRemoved,
      ObjectMoved,
    };

    Type m_EventType;
    const ezDocumentObjectBase* m_pObject;
    const ezDocumentObjectBase* m_pPreviousParent;
    const ezDocumentObjectBase* m_pNewParent;
  };

  ezEvent<const Event&> m_Events;

  const ezDocumentObjectBase* GetRootObject() const { return &m_RootObject; }

  void AddObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pParent);

  void RemoveObject(ezDocumentObjectBase* pObject);

  void MoveObject(ezDocumentObjectBase* pObject, ezDocumentObjectBase* pNewParent);

  const ezDocumentObjectBase* GetObject(const ezUuid& guid) const;

private:
  ezDocumentObjectRoot m_RootObject;

  /// \todo this should be a hash map
  ezMap<ezUuid, const ezDocumentObjectBase*> m_GuidToObject;
};
