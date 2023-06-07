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
#  if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  ezAllocator<ezMemoryPolicies::ezHeapAllocation, ezMemoryTrackingFlags::RegisterAllocator | ezMemoryTrackingFlags::EnableAllocationTracking>
    m_Allocator;
#  else
  ezAllocator<ezMemoryPolicies::ezHeapAllocation, ezMemoryTrackingFlags::None> m_Allocator;
#  endif
};

#endif // BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT
