#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/CommonAllocators.h>
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
    Object = EZ_BIT(6)     ///< ECMAScript object: includes objects, arrays, functions, threads
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



class EZ_CORE_DLL ezDuktapeWrapper
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezDuktapeWrapper);

public:
  ezDuktapeWrapper(const char* szWrapperName);
  ~ezDuktapeWrapper();

  /// \name Basics
  ///@{

  /// \brief Returns the raw Duktape context for custom operations.
  duk_context* GetContext() const { return m_pContext; }

  /// \brief Implicit conversion to duk_context*
  operator duk_context*() const { return m_pContext; }

  /// \brief Enables support for loading modules via the 'require' function
  void EnableModuleSupport(duk_c_function pModuleSearchFunction);

  ///@}

  /// \name Executing Scripts
  ///@{

  ezResult ExecuteString(const char* szString, const char* szDebugName = "eval");

  ezResult ExecuteStream(ezStreamReader& stream, const char* szDebugName);

  ezResult ExecuteFile(const char* szFile);

  ///@}

  /// \name C Functions
  ///@{

  void RegisterFunction(const char* szFunctionName, duk_c_function pFunction, ezUInt8 uiNumArguments, ezInt16 iMagicValue = 0);

  void RegisterFunctionWithVarArgs(const char* szFunctionName, duk_c_function pFunction, ezInt16 iMagicValue = 0);


  ezResult BeginFunctionCall(const char* szFunctionName);
  ezResult ExecuteFunctionCall();
  void EndFunctionCall();

  void PushParameter(ezInt32 iParam);
  void PushParameter(bool bParam);
  void PushParameter(double fParam);
  void PushParameter(const char* szParam);
  void PushParameter(const char* szParam, ezUInt32 length);
  void PushParameterNull();
  void PushParameterUndefined();

  bool GetBoolReturnValue(bool fallback = false) const;
  ezInt32 GetIntReturnValue(ezInt32 fallback = 0) const;
  float GetFloatReturnValue(float fallback = 0) const;
  double GetNumberReturnValue(double fallback = 0) const;
  const char* GetStringReturnValue(const char* fallback = "") const;

  bool IsReturnValueOfType(ezBitflags<ezDuktapeTypeMask> mask) const;
  bool IsReturnValueBool() const;
  bool IsReturnValueNumber() const;
  bool IsReturnValueString() const;
  bool IsReturnValueNull() const;
  bool IsReturnValueUndefined() const;
  bool IsReturnValueObject() const;


  ///@}

  /// \name Working with Objects
  ///@{

  ezResult OpenObject(const char* szObjectName);
  void OpenGlobalObject();
  void OpenGlobalStashObject();
  void CloseObject();

  bool HasProperty(const char* szPropertyName);

  bool GetBoolProperty(const char* szPropertyName, bool fallback);
  ezInt32 GetIntProperty(const char* szPropertyName, ezInt32 fallback);
  float GetFloatProperty(const char* szPropertyName, float fallback);
  double GetNumberProperty(const char* szPropertyName, double fallback);
  const char* GetStringProperty(const char* szPropertyName, const char* fallback);

  ///@}

private:
  void InitializeContext();
  void DestroyContext();

  static void FatalErrorHandler(void* pUserData, const char* szMsg);
  static void* DukAlloc(void* pUserData, size_t size);
  static void* DukRealloc(void* pUserData, void* pPointer, size_t size);
  static void DukFree(void* pUserData, void* pPointer);


protected:
  ezDuktapeWrapper(duk_context* pExistingContext);


  struct States
  {
    ezInt32 m_iPushedFunctionArguments = 0;
    ezInt32 m_iOpenObjects = 0;
    bool m_bAutoOpenedGlobalObject = false;
  };

  bool m_bIsInFunctionCall = false;
  bool m_bInitializedModuleSupport = false;
  States m_States;

private:
  /// If this script created the context, it also releases it on exit.
  bool m_bReleaseOnExit = true;

  duk_context* m_pContext = nullptr;

#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  ezAllocator<ezMemoryPolicies::ezHeapAllocation, ezMemoryTrackingFlags::EnableTracking> m_Allocator;
#  else
  ezAllocator<ezMemoryPolicies::ezHeapAllocation, ezMemoryTrackingFlags::None> m_Allocator;
#  endif
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_CORE_DLL ezDuktapeFunction final : public ezDuktapeWrapper
{
public:
  ezDuktapeFunction(duk_context* pExistingContext);
  ~ezDuktapeFunction();

  /// \name Retrieving function parameters
  ///@{

  /// Returns how many Parameters were passed to the called C-Function.
  ezUInt32 GetNumVarArgFunctionParameters() const;

  ezInt16 GetFunctionMagicValue() const;

  bool GetBoolParameter(ezUInt32 uiArgIdx, bool fallback = false) const;
  ezInt32 GetIntParameter(ezUInt32 uiArgIdx, ezInt32 fallback = 0) const;
  float GetFloatParameter(ezUInt32 uiArgIdx, float fallback = 0) const;
  double GetNumberParameter(ezUInt32 uiArgIdx, double fallback = 0) const;
  const char* GetStringParameter(ezUInt32 uiArgIdx, const char* fallback = "") const;

  bool IsParameterOfType(ezUInt32 uiArgIdx, ezBitflags<ezDuktapeTypeMask> mask) const;
  bool IsParameterBool(ezUInt32 uiArgIdx) const;
  bool IsParameterNumber(ezUInt32 uiArgIdx) const;
  bool IsParameterString(ezUInt32 uiArgIdx) const;
  bool IsParameterNull(ezUInt32 uiArgIdx) const;
  bool IsParameterUndefined(ezUInt32 uiArgIdx) const;
  bool IsParameterObject(ezUInt32 uiArgIdx) const;

  ///@}

  /// \name Returning values from C function
  ///@{

  ezInt32 ReturnVoid();
  ezInt32 ReturnNull();
  ezInt32 ReturnUndefined();
  ezInt32 ReturnBool(bool value);
  ezInt32 ReturnInt(ezInt32 value);
  ezInt32 ReturnFloat(float value);
  ezInt32 ReturnNumber(double value);
  ezInt32 ReturnString(const char* value);
  ezInt32 ReturnCustom();

  ///@}

private:
  bool m_bDidReturnValue = false;
};

//////////////////////////////////////////////////////////////////////////

class EZ_CORE_DLL ezDuktapeStackValidator
{
public:
  ezDuktapeStackValidator(duk_context* pContext);
  ~ezDuktapeStackValidator();

private:
  duk_context* m_pContext = nullptr;
  ezInt32 m_iStackTop = 0;
};

#  include <Core/Scripting/DuktapeWrapper/DuktapeWrapper.inl>

#endif // BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT
