#pragma once

#include <EditorFramework/Plugin.h>
#include <Foundation/Reflection/Reflection.h>

struct EZ_EDITORFRAMEWORK_DLL ezExposedParameter
{
  ezString m_sName;
  ezVariant m_DefaultValue;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_EDITORFRAMEWORK_DLL, ezExposedParameter)

class EZ_EDITORFRAMEWORK_DLL ezExposedParameters : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezExposedParameters, ezReflectedClass);
public:
  ezExposedParameters();
  ezDynamicArray<ezExposedParameter> m_Parameters;
};
