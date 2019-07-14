#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

struct EZ_EDITORFRAMEWORK_DLL ezExposedParameter
{
  ezExposedParameter();
  virtual ~ezExposedParameter();

  ezString m_sName;
  ezString m_sType;
  ezVariant m_DefaultValue;
  ezHybridArray<ezPropertyAttribute*, 2> m_Attributes;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezExposedParameter)

class EZ_EDITORFRAMEWORK_DLL ezExposedParameters : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExposedParameters, ezReflectedClass);
public:
  ezExposedParameters();
  virtual ~ezExposedParameters();

  const ezExposedParameter* Find(const char* szParamName) const;

  ezDynamicArray<ezExposedParameter*> m_Parameters;
};
