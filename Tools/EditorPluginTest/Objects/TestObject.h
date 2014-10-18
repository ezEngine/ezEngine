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

  ezTestEditorProperties m_EditorProps;
};

class ezTestObject : public ezDocumentObjectBase
{
public:
  ezTestObject();

  virtual const ezIReflectedTypeAccessor& GetTypeAccessor() const override { return m_ObjectProperties; }
  virtual const ezIReflectedTypeAccessor& GetEditorTypeAccessor() const override { return m_EditorProperties; }

private:
  ezTestObjectProperties m_TestObjectProperties;
  ezTestEditorProperties m_TestEditorProperties;

  ezReflectedTypeDirectAccessor m_ObjectProperties;
  ezReflectedTypeDirectAccessor m_EditorProperties;
};