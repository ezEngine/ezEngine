#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>

class ezSceneObjectEditorProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSceneObjectEditorProperties);

public:
  ezSceneObjectEditorProperties();

  void SetName(const char* szName) { m_sName.Assign(szName); }
  const char* GetName() const { return m_sName.GetString().GetData(); }

protected:
  ezHashedString m_sName;
};

struct ezExampleEnum
{
  typedef ezInt8 StorageType;
  enum Enum
  {
    Value1 = 0,          // normal value
    Value2 = -2,         // normal value
    Value3 = 4,          // normal value
    Default = Value1     // Default initialization value (required)
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezExampleEnum);

struct ezExampleBitflags
{
  typedef ezUInt64 StorageType;
  enum Enum : ezUInt64
  {
    Value1 = EZ_BIT(0),  // normal value
    Value2 = EZ_BIT(31), // normal value
    Value3 = EZ_BIT(63), // normal value
    Default = Value1     // Default initialization value (required)
  };

  struct Bits
  {
    StorageType Value1 : 1;
    StorageType Padding : 30;
    StorageType Value2 : 1;
    StorageType Padding2 : 31;
    StorageType Value3 : 1;
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ezExampleBitflags);


class ezTestObjectProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestObjectProperties);

public:
  ezTestObjectProperties();

  float m_fValue;
  ezInt8 m_Int8;
  ezInt16 m_Int16;
  ezUInt32 m_UInt32;
  ezColor m_Color;
  ezEnum<ezExampleEnum> m_Enum;
  ezBitflags<ezExampleBitflags> m_Bitflags;

  ezSceneObjectEditorProperties m_EditorProps;
};

class ezTestObject : public ezDocumentObjectDirectPtr<ezSceneObjectEditorProperties>
{
public:
  ezTestObject(ezReflectedClass* pObjectProperties) : ezDocumentObjectDirectPtr<ezSceneObjectEditorProperties>(pObjectProperties)
  {
  }

  ~ezTestObject()
  {
    delete m_pObjectProperties;
    m_pObjectProperties = nullptr;
  }
};

class ezTestObject2 : public ezDocumentObjectStorage<ezSceneObjectEditorProperties>
{
public:
  ezTestObject2(ezRTTI* hObjectProperties) : ezDocumentObjectStorage<ezSceneObjectEditorProperties>(hObjectProperties)
  {
  }

};

