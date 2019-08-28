#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

struct duk_hthread;
typedef duk_hthread duk_context;
typedef int (*duk_c_function)(duk_context* ctx);

class EZ_CORE_DLL ezDuktapeWrapper
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezDuktapeWrapper);

public:
  ezDuktapeWrapper(const char* szWrapperName);
  ~ezDuktapeWrapper();

  /// \name Basics
  ///@{

  /// \brief Returns the raw Duktape context for custom operations.
  duk_context* GetContext();

  /// \brief Returns the raw Duktape context for custom operations.
  const duk_context* GetContext() const;

  ///@}

  /// \name Executing Scripts
  ///@{

  ezResult ExecuteString(const char* szString);

  ///@}

  /// \name C Functions
  ///@{

  void RegisterFunction(const char* szFunctionName, duk_c_function pFunction, ezUInt8 uiNumArguments);

  bool PrepareFunctionCall(const char* szFunctionName);

  ezResult CallPreparedFunction();

  void PushParameter(ezInt32 iParam);
  void PushParameter(bool bParam);
  void PushParameter(double fParam);
  void PushParameter(const char* szParam);
  void PushParameter(const char* szParam, ezUInt32 length);
  void PushParameterNull();
  void PushParameterUndefined();

  ///@}

private:
  void InitializeContext();
  void DestroyContext();

  static void FatalErrorHandler(void* pUserData, const char* szMsg);
  static void* DukAlloc(void* pUserData, size_t size);
  static void* DukRealloc(void* pUserData, void* pPointer, size_t size);
  static void DukFree(void* pUserData, void* pPointer);


private:
  ezString m_sWrapperName;
  duk_context* m_pContext = nullptr;
  mutable ezProxyAllocator m_Allocator;

  struct States
  {
    ezInt32 m_iPushedFunctionArguments = 0;
  };

  States m_States;
};

#  include <Core/Scripting/DuktapeWrapper/DuktapeWrapper.inl>

#endif // BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT
