#include <CorePCH.h>

#include <Core/Scripting/DuktapeWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Duktape/duk_module_duktape.h>
#  include <Duktape/duktape.h>
#  include <Foundation/IO/FileSystem/FileReader.h>



ezDuktapeWrapper::ezDuktapeWrapper(const char* szWrapperName)
  : ezDuktapeHelper(nullptr)
  , m_Allocator(szWrapperName, ezFoundation::GetDefaultAllocator())

{
  InitializeContext();
}

ezDuktapeWrapper::ezDuktapeWrapper(duk_context* pExistingContext)
  : ezDuktapeHelper(pExistingContext)
  , m_Allocator("", nullptr)
{
  EZ_ASSERT_ALWAYS(pExistingContext != nullptr, "Duktape context must not be null");

  m_bReleaseOnExit = false;
}

ezDuktapeWrapper::~ezDuktapeWrapper()
{
  DestroyContext();
}

void ezDuktapeWrapper::EnableModuleSupport(duk_c_function pModuleSearchFunction)
{
  if (!m_bInitializedModuleSupport)
  {
    // we need an 'exports' object for the transpiled TypeScript files to load
    // (they access this object to define the '__esModule' property on it)
    ExecuteString("var exports = {}");

    m_bInitializedModuleSupport = true;
    duk_module_duktape_init(m_pContext);
  }

  if (pModuleSearchFunction)
  {
    duk_get_global_string(m_pContext, "Duktape");
    duk_push_c_function(m_pContext, pModuleSearchFunction, 4);
    duk_put_prop_string(m_pContext, -2, "modSearch");
    duk_pop(m_pContext);
  }
}

void ezDuktapeWrapper::InitializeContext()
{
  EZ_ASSERT_ALWAYS(m_pContext == nullptr, "Duktape context should be null");

  m_bReleaseOnExit = true;

  m_pContext = duk_create_heap(DukAlloc, DukRealloc, DukFree, this, FatalErrorHandler);

  EZ_ASSERT_ALWAYS(m_pContext != nullptr, "Duktape context could not be created");
}

void ezDuktapeWrapper::DestroyContext()
{
  if (!m_bReleaseOnExit)
    return;

  duk_destroy_heap(m_pContext);
  m_pContext = nullptr;

  const auto stats = m_Allocator.GetStats();
  EZ_ASSERT_DEBUG(stats.m_uiAllocationSize == 0, "Duktape did not free all data");
  EZ_ASSERT_DEBUG(stats.m_uiNumAllocations == stats.m_uiNumDeallocations, "Duktape did not free all data");
}

void ezDuktapeWrapper::FatalErrorHandler(void* pUserData, const char* szMsg)
{
  EZ_REPORT_FAILURE(szMsg);
}

void* ezDuktapeWrapper::DukAlloc(void* pUserData, size_t size)
{
  EZ_ASSERT_DEBUG(size > 0, "Invalid allocation");

  ezDuktapeWrapper* pDukWrapper = reinterpret_cast<ezDuktapeWrapper*>(pUserData);

  return pDukWrapper->m_Allocator.Allocate(size, 8);
}

void* ezDuktapeWrapper::DukRealloc(void* pUserData, void* pPointer, size_t size)
{
  ezDuktapeWrapper* pDukWrapper = reinterpret_cast<ezDuktapeWrapper*>(pUserData);

  if (size == 0)
  {
    if (pPointer != nullptr)
    {
      pDukWrapper->m_Allocator.Deallocate(pPointer);
    }

    return nullptr;
  }

  if (pPointer == nullptr)
  {
    return pDukWrapper->m_Allocator.Allocate(size, 8);
  }
  else
  {
    return pDukWrapper->m_Allocator.Reallocate(pPointer, pDukWrapper->m_Allocator.AllocatedSize(pPointer), size, 8);
  }
}

void ezDuktapeWrapper::DukFree(void* pUserData, void* pPointer)
{
  if (pPointer == nullptr)
    return;

  ezDuktapeWrapper* pDukWrapper = reinterpret_cast<ezDuktapeWrapper*>(pUserData);

  pDukWrapper->m_Allocator.Deallocate(pPointer);
}

ezDuktapeStackValidator::ezDuktapeStackValidator(duk_context* pContext, ezInt32 iExpectedChange /*= 0*/)
{
  m_pContext = pContext;
  m_iStackTop = duk_get_top(m_pContext) + iExpectedChange;
}

ezDuktapeStackValidator::~ezDuktapeStackValidator()
{
  const int iCurTop = duk_get_top(m_pContext);
  EZ_ASSERT_DEBUG(iCurTop == m_iStackTop, "Stack top is not as expected");
}

void ezDuktapeStackValidator::AdjustExpected(ezInt32 iChange)
{
  m_iStackTop += iChange;
}

#endif
