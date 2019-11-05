#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

struct duk_hthread;
typedef duk_hthread duk_context;
typedef int (*duk_c_function)(duk_context* ctx);

struct ezDuktapeTypeMask
{
  typedef ezUInt32 StorageType;

  enum Enum
  {
    None = EZ_BIT(0),      ///< no value, e.g. invalid index
    Undefined = EZ_BIT(1), ///< ECMAScript undefined
    Null = EZ_BIT(2),      ///< ECMAScript null
    Bool = EZ_BIT(3),      ///< boolean, true or false
    Number = EZ_BIT(4),    ///< any number, stored as a double
    String = EZ_BIT(5),    ///< ECMAScript string: CESU-8 / extended UTF-8 encoded
    Object = EZ_BIT(6),    ///< ECMAScript object: includes objects, arrays, functions, threads
    Buffer = EZ_BIT(7),    ///< fixed or dynamic, garbage collected byte buffer
    Pointer = EZ_BIT(8)    ///< raw void pointer

  };

  struct Bits
  {
    StorageType None : 1;
    StorageType Undefined : 1;
    StorageType Null : 1;
    StorageType Bool : 1;
    StorageType Number : 1;
    StorageType String : 1;
    StorageType Object : 1;
  };
};

EZ_DECLARE_FLAGS_OPERATORS(ezDuktapeTypeMask);

class EZ_CORE_DLL ezDuktapeHelper
{
public:
  ezDuktapeHelper(duk_context* pContext, ezInt32 iExpectedStackChange);
  ~ezDuktapeHelper();

  /// \name Basics
  ///@{

  /// \brief Returns the raw Duktape context for custom operations.
  duk_context* GetContext() const { return m_pContext; }

  /// \brief Implicit conversion to duk_context*
  operator duk_context*() const { return m_pContext; }

  void SetExpectedStackChange(ezInt32 iExpectedStackChange /*= -0xFFFF to disable stack check */);

  ///@}
  /// \name Error Handling
  ///@{

  void Error(ezFormatString text);


  ///@}
  /// \name Objects / Stash
  ///@{

  void PopStack(ezUInt32 n = 1);

  void PushGlobalObject();

  void PushGlobalStash();

  ezResult PushLocalObject(const char* szName, ezInt32 iParentObjectIndex = -1);

  ///@}
  /// \name Object Properties
  ///@{

  bool HasProperty(const char* szPropertyName, ezInt32 iParentObjectIndex = -1) const;

  bool GetBoolProperty(const char* szPropertyName, bool fallback, ezInt32 iParentObjectIndex = -1) const;
  ezInt32 GetIntProperty(const char* szPropertyName, ezInt32 fallback, ezInt32 iParentObjectIndex = -1) const;
  ezUInt32 GetUIntProperty(const char* szPropertyName, ezUInt32 fallback, ezInt32 iParentObjectIndex = -1) const;
  float GetFloatProperty(const char* szPropertyName, float fallback, ezInt32 iParentObjectIndex = -1) const;
  double GetNumberProperty(const char* szPropertyName, double fallback, ezInt32 iParentObjectIndex = -1) const;
  const char* GetStringProperty(const char* szPropertyName, const char* fallback, ezInt32 iParentObjectIndex = -1) const;

  void SetBoolProperty(const char* szPropertyName, bool value, ezInt32 iParentObjectIndex = -1) const;
  void SetNumberProperty(const char* szPropertyName, double value, ezInt32 iParentObjectIndex = -1) const;
  void SetStringProperty(const char* szPropertyName, const char* value, ezInt32 iParentObjectIndex = -1) const;
  void SetCustomProperty(const char* szPropertyName, ezInt32 iParentObjectIndex = -1) const;


  ///@}
  /// \name Global State
  ///@{

  void StorePointerInStash(const char* szKey, void* pPointer);
  void* RetrievePointerFromStash(const char* szKey) const;

  void StoreStringInStash(const char* szKey, const char* value);
  const char* RetrieveStringFromStash(const char* szKey, const char* szFallback = nullptr) const;

  ///@}
  /// \name Type Checks
  ///@{

  bool IsOfType(ezBitflags<ezDuktapeTypeMask> mask, ezInt32 iStackElement = -1) const;
  bool IsBool(ezInt32 iStackElement = -1) const;
  bool IsNumber(ezInt32 iStackElement = -1) const;
  bool IsString(ezInt32 iStackElement = -1) const;
  bool IsNull(ezInt32 iStackElement = -1) const;
  bool IsUndefined(ezInt32 iStackElement = -1) const;
  bool IsObject(ezInt32 iStackElement = -1) const;
  bool IsBuffer(ezInt32 iStackElement = -1) const;
  bool IsPointer(ezInt32 iStackElement = -1) const;
  bool IsNullOrUndefined(ezInt32 iStackElement = -1) const;

  ///@}
  /// \name C Functions
  ///@{

  void RegisterGlobalFunction(const char* szFunctionName, duk_c_function pFunction, ezUInt8 uiNumArguments, ezInt16 iMagicValue = 0);
  void RegisterGlobalFunctionWithVarArgs(const char* szFunctionName, duk_c_function pFunction, ezInt16 iMagicValue = 0);

  void RegisterObjectFunction(const char* szFunctionName, duk_c_function pFunction, ezUInt8 uiNumArguments, ezInt32 iParentObjectIndex = -1, ezInt16 iMagicValue = 0);

  ezResult PrepareGlobalFunctionCall(const char* szFunctionName);
  ezResult PrepareObjectFunctionCall(const char* szFunctionName, ezInt32 iParentObjectIndex = -1);
  ezResult CallPreparedFunction();

  ezResult PrepareMethodCall(const char* szMethodName, ezInt32 iParentObjectIndex = -1);
  ezResult CallPreparedMethod();


  ///@}
  /// \name Values / Parameters
  ///@{

  void PushInt(ezInt32 iParam);
  void PushUInt(ezUInt32 uiParam);
  void PushBool(bool bParam);
  void PushNumber(double fParam);
  void PushString(const ezStringView& sParam);
  void PushNull();
  void PushUndefined();
  void PushCustom(ezUInt32 num = 1);

  bool GetBoolValue(ezInt32 iStackElement, bool fallback = false) const;
  ezInt32 GetIntValue(ezInt32 iStackElement, ezInt32 fallback = 0) const;
  ezUInt32 GetUIntValue(ezInt32 iStackElement, ezUInt32 fallback = 0) const;
  float GetFloatValue(ezInt32 iStackElement, float fallback = 0) const;
  double GetNumberValue(ezInt32 iStackElement, double fallback = 0) const;
  const char* GetStringValue(ezInt32 iStackElement, const char* fallback = "") const;

  ///@}
  /// \name Executing Scripts
  ///@{

  ezResult ExecuteString(const char* szString, const char* szDebugName = "eval");

  ezResult ExecuteStream(ezStreamReader& stream, const char* szDebugName);

  ezResult ExecuteFile(const char* szFile);

  ///@}

protected:
  duk_context* m_pContext = nullptr;
  ezInt32 m_iPushedValues = 0;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  ezInt32 m_iStackTopAtStart = 0;
  ezInt32 m_iExpectedStackChange = -10000;
#endif
};

#endif
