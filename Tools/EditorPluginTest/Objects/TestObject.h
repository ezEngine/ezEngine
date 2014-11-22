#pragma once

#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedTypeDirectAccessor.h>

class ezTestEditorProperties : public ezDocumentObjectBaseProperties
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTestEditorProperties);

public:
  ezTestEditorProperties();

  bool m_bAwesome;
  float m_fLoat;
};

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

  ezTestEditorProperties m_EditorProps;
};

class ezTestObject : public ezDocumentObjectDirect<ezTestEditorProperties>
{
public:
  ezTestObject(ezReflectedClass* pObjectProperties) : ezDocumentObjectDirect<ezTestEditorProperties>(pObjectProperties)
  {
  }

  ~ezTestObject()
  {
    delete m_pObjectProperties;
    m_pObjectProperties = nullptr;
  }
};

class ezTestObject2 : public ezDocumentObjectStorage<ezTestEditorProperties>
{
public:
  ezTestObject2(ezReflectedTypeHandle hObjectProperties) : ezDocumentObjectStorage<ezTestEditorProperties>(hObjectProperties)
  {
  }

};

