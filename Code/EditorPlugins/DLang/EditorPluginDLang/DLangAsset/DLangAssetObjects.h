#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class ezDLangParameter : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDLangParameter, ezReflectedClass);

public:
  ezString m_sName;
};

class ezDLangParameterNumber : public ezDLangParameter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDLangParameterNumber, ezDLangParameter);

public:
  double m_DefaultValue = 0;
};

class ezDLangParameterBool : public ezDLangParameter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDLangParameterBool, ezDLangParameter);

public:
  bool m_DefaultValue = false;
};

class ezDLangParameterString : public ezDLangParameter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDLangParameterString, ezDLangParameter);

public:
  ezString m_DefaultValue;
};

class ezDLangParameterVec3 : public ezDLangParameter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDLangParameterVec3, ezDLangParameter);

public:
  ezVec3 m_DefaultValue = ezVec3(0.0f);
};

class ezDLangParameterColor : public ezDLangParameter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDLangParameterColor, ezDLangParameter);

public:
  ezColor m_DefaultValue = ezColor::White;
};


class ezDLangAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDLangAssetProperties, ezReflectedClass);

public:
  ezDLangAssetProperties();
  ~ezDLangAssetProperties();

  ezString m_sScriptFile;

  ezDynamicArray<ezDLangParameterNumber> m_NumberParameters;
  ezDynamicArray<ezDLangParameterBool> m_BoolParameters;
  ezDynamicArray<ezDLangParameterString> m_StringParameters;
  ezDynamicArray<ezDLangParameterVec3> m_Vec3Parameters;
  ezDynamicArray<ezDLangParameterColor> m_ColorParameters;
};
