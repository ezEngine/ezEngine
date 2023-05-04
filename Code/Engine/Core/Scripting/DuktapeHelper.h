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
  using StorageType = ezUInt32;

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

#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)

#    define EZ_DUK_VERIFY_STACK(duk, ExpectedStackChange) \
      duk.EnableStackChangeVerification();                \
      duk.VerifyExpectedStackChange(ExpectedStackChange, EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION);

#    define EZ_DUK_RETURN_AND_VERIFY_STACK(duk, ReturnCode, ExpectedStackChange) \
      {                                                                          \
        auto ret = ReturnCode;                                                   \
        EZ_DUK_VERIFY_STACK(duk, ExpectedStackChange);                           \
        return ret;                                                              \
      }

#    define EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, ExpectedStackChange) \
      EZ_DUK_VERIFY_STACK(duk, ExpectedStackChange);                      \
      return;


#  else

#    define EZ_DUK_VERIFY_STACK(duk, ExpectedStackChange)

#    define EZ_DUK_RETURN_AND_VERIFY_STACK(duk, ReturnCode, ExpectedStackChange) return ReturnCode;

#    define EZ_DUK_RETURN_VOID_AND_VERIFY_STACK(duk, ExpectedStackChange) return;

#  endif

class EZ_CORE_DLL ezDuktapeHelper
{
public:
  ezDuktapeHelper(duk_context* pContext);
  ezDuktapeHelper(const ezDuktapeHelper& rhs);
  ~ezDuktapeHelper();
  void operator=(const ezDuktapeHelper& rhs);

  /// \name Basics
  ///@{

  /// \brief Returns the raw Duktape context for custom operations.
  EZ_ALWAYS_INLINE duk_context* GetContext() const { return m_pContext; }

  /// \brief Implicit conversion to duk_context*
  EZ_ALWAYS_INLINE operator duk_context*() const { return m_pContext; }

#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  void VerifyExpectedStackChange(ezInt32 iExpectedStackChange, const char* szFile, ezUInt32 uiLine, const char* szFunction) const;
#  endif

  ///@}
  /// \name Error Handling
  ///@{

  void Error(const ezFormatString& text);

  void LogStackTrace(ezInt32 iErrorObjIdx);


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

  bool GetBoolProperty(const char* szPropertyName, bool bFallback, ezInt32 iParentObjectIndex = -1) const;
  ezInt32 GetIntProperty(const char* szPropertyName, ezInt32 iFallback, ezInt32 iParentObjectIndex = -1) const;
  ezUInt32 GetUIntProperty(const char* szPropertyName, ezUInt32 uiFallback, ezInt32 iParentObjectIndex = -1) const;
  float GetFloatProperty(const char* szPropertyName, float fFallback, ezInt32 iParentObjectIndex = -1) const;
  double GetNumberProperty(const char* szPropertyName, double fFallback, ezInt32 iParentObjectIndex = -1) const;
  const char* GetStringProperty(const char* szPropertyName, const char* szFallback, ezInt32 iParentObjectIndex = -1) const;

  void SetBoolProperty(const char* szPropertyName, bool value, ezInt32 iParentObjectIndex = -1) const;
  void SetNumberProperty(const char* szPropertyName, double value, ezInt32 iParentObjectIndex = -1) const;
  void SetStringProperty(const char* szPropertyName, const char* value, ezInt32 iParentObjectIndex = -1) const;

  /// \note If a negative parent index is given, the parent object taken is actually ParentIdx - 1 (obj at idx -1 is the custom object to use)
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

  void RegisterGlobalFunction(const char* szFunctionName, duk_c_function function, ezUInt8 uiNumArguments, ezInt16 iMagicValue = 0);
  void RegisterGlobalFunctionWithVarArgs(const char* szFunctionName, duk_c_function function, ezInt16 iMagicValue = 0);

  void RegisterObjectFunction(
    const char* szFunctionName, duk_c_function function, ezUInt8 uiNumArguments, ezInt32 iParentObjectIndex = -1, ezInt16 iMagicValue = 0);

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
  void PushCustom(ezUInt32 uiNum = 1);

  bool GetBoolValue(ezInt32 iStackElement, bool bFallback = false) const;
  ezInt32 GetIntValue(ezInt32 iStackElement, ezInt32 iFallback = 0) const;
  ezUInt32 GetUIntValue(ezInt32 iStackElement, ezUInt32 uiFallback = 0) const;
  float GetFloatValue(ezInt32 iStackElement, float fFallback = 0) const;
  double GetNumberValue(ezInt32 iStackElement, double fFallback = 0) const;
  const char* GetStringValue(ezInt32 iStackElement, const char* szFallback = "") const;

  ///@}
  /// \name Executing Scripts
  ///@{

  ezResult ExecuteString(const char* szString, const char* szDebugName = "eval");

  ezResult ExecuteStream(ezStreamReader& inout_stream, const char* szDebugName);

  ezResult ExecuteFile(const char* szFile);

  ///@}

public:
#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  void EnableStackChangeVerification() const;
#  endif


protected:
  duk_context* m_pContext = nullptr;
  ezInt32 m_iPushedValues = 0;

#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  ezInt32 m_iStackTopAtStart = -1000;
  mutable bool m_bVerifyStackChange = false;

#  endif
};

#endif
