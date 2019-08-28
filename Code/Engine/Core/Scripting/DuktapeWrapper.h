#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

struct duk_hthread;
typedef duk_hthread duk_context;

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
};

#  include <Core/Scripting/DuktapeWrapper/DuktapeWrapper.inl>

#endif // BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT
