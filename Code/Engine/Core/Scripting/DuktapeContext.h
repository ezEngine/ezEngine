#pragma once

#include <Core/Scripting/DuktapeFunction.h>
#include <Foundation/Memory/CommonAllocators.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

struct duk_hthread;
using duk_context = duk_hthread;
using duk_c_function = int (*)(duk_context*);


class EZ_CORE_DLL ezDuktapeContext : public ezDuktapeHelper
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezDuktapeContext);

public:
  ezDuktapeContext(ezStringView sWrapperName);
  ~ezDuktapeContext();

  /// \name Basics
  ///@{

  /// \brief Enables support for loading modules via the 'require' function
  void EnableModuleSupport(duk_c_function moduleSearchFunction);

  ///@}

private:
  void InitializeContext();
  void DestroyContext();

  static void FatalErrorHandler(void* pUserData, const char* szMsg);
  static void* DukAlloc(void* pUserData, size_t size);
  static void* DukRealloc(void* pUserData, void* pPointer, size_t size);
  static void DukFree(void* pUserData, void* pPointer);

protected:
  bool m_bInitializedModuleSupport = false;

private:
  // Careful! We always need AllocationStats, because DukRealloc calls AllocatedSize, which only works with these stats.
  ezAllocatorWithPolicy<ezAllocPolicyHeap, ezAllocatorTrackingMode::AllocationStats> m_Allocator;
};

#endif // BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT
