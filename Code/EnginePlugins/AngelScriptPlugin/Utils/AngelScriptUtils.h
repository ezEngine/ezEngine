#pragma once

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>

#include <Foundation/Basics.h>
#include <Foundation/Types/VariantType.h>

class asIScriptEngine;
class ezVariant;
class asIScriptGeneric;
class ezAbstractFunctionProperty;
class asIScriptModule;
class asIScriptFunction;

struct EZ_ANGELSCRIPTPLUGIN_DLL ezAsInfos
{
  ezSet<ezString> m_Types;
  ezSet<ezString> m_Namespaces;
  ezSet<ezString> m_GlobalFunctions;
  ezSet<ezString> m_Methods;
  ezSet<ezString> m_AllDeclarations;
  ezSet<ezString> m_Properties;
  ezSet<ezString> m_EnumValues;
};

class EZ_ANGELSCRIPTPLUGIN_DLL ezAngelScriptUtils
{
public:
  static void SetThreadLocalWorld(ezWorld* pWorld);
  static ezWorld* GetThreadLocalWorld();

  static void SaveByteCode(asIScriptModule* pModule, ezDynamicArray<ezUInt8>& out_byteCode);

  static const char* GetAsTypeName(asIScriptEngine* pEngine, int iAsTypeID);

  static ezString GetNiceFunctionDeclaration(const asIScriptFunction* pFunc, bool bIncludeObjectName = false, bool bIncludeNamespace = false);

  static asIScriptModule* LoadFromByteCode(asIScriptEngine* pEngine, ezStringView sModuleName, ezArrayPtr<ezUInt8> byteCode);

  static const ezRTTI* MapToRTTI(int iAsTypeID, asIScriptEngine* pEngine);

  static ezResult WriteToAsTypeAtLocation(asIScriptEngine* pEngine, int iAsTypeID, void* pMemoryLocation, const ezVariant& value);
  static ezResult ReadFromAsTypeAtLocation(asIScriptEngine* pEngine, int iAsTypeID, void* pMemoryLocation, ezVariant& out_value);

  static const char* VariantTypeToString(ezVariantType::Enum type);

  static ezString DefaultValueToString(const ezVariant& value);

  static void RetrieveArg(asIScriptGeneric* pGen, ezUInt32 uiRealArg, ezInt32& ref_iSkippedArg, const ezAbstractFunctionProperty* pAbstractFuncProp, ezVariant& out_arg);

  static void RetrieveVarArgs(asIScriptGeneric* pGen, ezUInt32 uiStartArg, const ezAbstractFunctionProperty* pAbstractFuncProp, ezVariant& out_arg);

  static void MakeGenericFunctionCall(asIScriptGeneric* pGen);

  static void DefaultConstructInPlace(void* pPtr, const ezRTTI* pRtti);

  static void RetrieveAsInfos(asIScriptEngine* pEngine, ezAsInfos& out_infos);

  static void GenerateAsPredefinedFile(asIScriptEngine* pEngine, ezStringBuilder& out_sContent);

  static ezString RegisterEnumType(asIScriptEngine* pEngine, const ezRTTI* pEnumType);

  static void RegisterTypeProperties(asIScriptEngine* pEngine, const char* szTypeName, const ezRTTI* pRtti, bool bIsInherited);
};
