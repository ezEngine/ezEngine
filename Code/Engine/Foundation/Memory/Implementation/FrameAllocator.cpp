#include <FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/StringBuilder.h>

ezDoubleBufferedStackAllocator::ezDoubleBufferedStackAllocator(const char* szName, ezAllocatorBase* pParent)
{
  ezStringBuilder sName = szName;
  sName.Append("0");

  m_pCurrentAllocator = EZ_DEFAULT_NEW(StackAllocatorType, sName, pParent);

  sName = szName;
  sName.Append("1");

  m_pOtherAllocator = EZ_DEFAULT_NEW(StackAllocatorType, sName, pParent);
}

ezDoubleBufferedStackAllocator::~ezDoubleBufferedStackAllocator()
{
  EZ_DEFAULT_DELETE(m_pCurrentAllocator);
  EZ_DEFAULT_DELETE(m_pOtherAllocator);
}

void ezDoubleBufferedStackAllocator::Swap()
{
  ezMath::Swap(m_pCurrentAllocator, m_pOtherAllocator);

  m_pCurrentAllocator->Reset();
}

void ezDoubleBufferedStackAllocator::Reset()
{
  m_pCurrentAllocator->Reset();
  m_pOtherAllocator->Reset();
}


// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, FrameAllocator)

  ON_CORESYSTEMS_STARTUP
  {
    ezFrameAllocator::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezFrameAllocator::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezDoubleBufferedStackAllocator* ezFrameAllocator::s_pAllocator;

// static
void ezFrameAllocator::Swap()
{
  EZ_PROFILE_SCOPE("FrameAllocator.Swap");

  s_pAllocator->Swap();
}

// static
void ezFrameAllocator::Reset()
{
  if (s_pAllocator)
  {
    s_pAllocator->Reset();
  }
}

// static
void ezFrameAllocator::Startup()
{
  s_pAllocator = EZ_DEFAULT_NEW(ezDoubleBufferedStackAllocator, "FrameAllocator", ezFoundation::GetAlignedAllocator());
}

// static
void ezFrameAllocator::Shutdown()
{
  EZ_DEFAULT_DELETE(s_pAllocator);
}

EZ_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_FrameAllocator);
