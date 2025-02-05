#pragma once

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>

#include <Foundation/Basics.h>
#include <Foundation/Types/VariantType.h>

class asIScriptEngine;
class ezVariant;
class asIScriptGeneric;
class ezAbstractFunctionProperty;

class EZ_ANGELSCRIPTPLUGIN_DLL ezAngelScriptUtils
{
public:
  static const ezRTTI* MapToRTTI(int iAsTypeID, asIScriptEngine* pEngine);

  static ezResult WriteToAsTypeLocation(asIScriptEngine* pEngine, int iAsTypeID, void* pMemoryLocation, const ezVariant& value);
  static ezResult ReadFromAsTypeLocation(asIScriptEngine* pEngine, int iAsTypeID, void* pMemoryLocation, ezVariant& out_Value);

  static const char* VariantTypeToString(ezVariantType::Enum type);

  static ezString DefaultValueToString(const ezVariant& value);

  static void RetrieveArg(asIScriptGeneric* gen, ezUInt32 uiArg, const ezAbstractFunctionProperty* pAbstractFuncProp, ezVariant& out_arg);

  static void RetrieveVarArgs(asIScriptGeneric* gen, ezUInt32 uiStartArg, const ezAbstractFunctionProperty* pAbstractFuncProp, ezVariant& out_arg);

  static void MakeGenericFunctionCall(asIScriptGeneric* gen);

  static void DefaultConstructInPlace(void* ptr, const ezRTTI* pRtti);
};
