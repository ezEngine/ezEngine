#include <CorePCH.h>

#include <Core/Scripting/DuktapeWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Duktape/duktape.h>

ezDuktapeWrapper::ezDuktapeWrapper(const char* szWrapperName)
  : m_Allocator(szWrapperName, ezFoundation::GetDefaultAllocator())
{
  m_sWrapperName = szWrapperName;

  InitializeContext();
}

ezDuktapeWrapper::~ezDuktapeWrapper()
{
  DestroyContext();
}

duk_context* ezDuktapeWrapper::GetContext()
{
  return m_pContext;
}

const duk_context* ezDuktapeWrapper::GetContext() const
{
  return m_pContext;
}

ezResult ezDuktapeWrapper::ExecuteString(const char* szString)
{
  if (duk_peval_string(m_pContext, szString) == 0)
  {
    duk_pop(m_pContext);
    return EZ_SUCCESS;
  }

  // TODO: distinguish between compile and eval errors

  {
    EZ_LOG_BLOCK("ezDuktapeWrapper::ExecuteString");

    ezLog::Error("[duktape]{}", duk_safe_to_string(m_pContext, -1));
    ezLog::Info("[duktape]Script: {0}", szString);
  }

  return EZ_FAILURE;
}

void ezDuktapeWrapper::InitializeContext()
{
  m_pContext = duk_create_heap(DukAlloc, DukRealloc, DukFree, this, FatalErrorHandler);

  EZ_ASSERT_ALWAYS(m_pContext != nullptr, "Duktape context could not be created");
}

void ezDuktapeWrapper::DestroyContext()
{
  duk_destroy_heap(m_pContext);
  m_pContext = nullptr;

  const auto stats = m_Allocator.GetStats();
  EZ_ASSERT_DEBUG(stats.m_uiAllocationSize == 0, "Duktape did not free all data");
  EZ_ASSERT_DEBUG(stats.m_uiNumAllocations == stats.m_uiNumDeallocations, "Duktape did not free all data");
}

void ezDuktapeWrapper::FatalErrorHandler(void* pUserData, const char* szMsg)
{
  ezDuktapeWrapper* pDukWrapper = reinterpret_cast<ezDuktapeWrapper*>(pUserData);

  EZ_REPORT_FAILURE("{}: {}", pDukWrapper->m_sWrapperName, szMsg);
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

#endif
