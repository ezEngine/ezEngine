#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class ezTypeScriptParameter : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptParameter, ezReflectedClass);

public:
  ezString m_sName;
};

class ezTypeScriptParameterNumber : public ezTypeScriptParameter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptParameterNumber, ezTypeScriptParameter);

public:
  double m_DefaultValue = 0;
};

class ezTypeScriptParameterBool : public ezTypeScriptParameter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptParameterBool, ezTypeScriptParameter);

public:
  bool m_DefaultValue = false;
};

class ezTypeScriptParameterString : public ezTypeScriptParameter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptParameterString, ezTypeScriptParameter);

public:
  ezString m_DefaultValue;
};

class ezTypeScriptParameterVec3 : public ezTypeScriptParameter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptParameterVec3, ezTypeScriptParameter);

public:
  ezVec3 m_DefaultValue = ezVec3(0.0f);
};

class ezTypeScriptParameterColor : public ezTypeScriptParameter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptParameterColor, ezTypeScriptParameter);

public:
  ezColor m_DefaultValue = ezColor::White;
};


class ezTypeScriptAssetProperties : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezTypeScriptAssetProperties, ezReflectedClass);

public:
  ezTypeScriptAssetProperties();
  ~ezTypeScriptAssetProperties();

  ezString m_sScriptFile;

  ezDynamicArray<ezTypeScriptParameterNumber> m_NumberParameters;
  ezDynamicArray<ezTypeScriptParameterBool> m_BoolParameters;
  ezDynamicArray<ezTypeScriptParameterString> m_StringParameters;
  ezDynamicArray<ezTypeScriptParameterVec3> m_Vec3Parameters;
  ezDynamicArray<ezTypeScriptParameterColor> m_ColorParameters;

};
