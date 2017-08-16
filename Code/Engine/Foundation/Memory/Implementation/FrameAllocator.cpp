#include <PCH.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/StringBuilder.h>

ezDoubleBufferedStackAllocator::ezDoubleBufferedStackAllocator(ezAllocatorBase* pParent)
{
  m_pCurrentAllocator = EZ_NEW(pParent, StackAllocatorType, "StackAllocator0", pParent);
  m_pOtherAllocator = EZ_NEW(pParent, StackAllocatorType, "StackAllocator1", pParent);
}

ezDoubleBufferedStackAllocator::~ezDoubleBufferedStackAllocator()
{
  ezAllocatorBase* pParent = m_pCurrentAllocator->GetParent();
  EZ_DELETE(pParent, m_pCurrentAllocator);
  EZ_DELETE(pParent, m_pOtherAllocator);
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



EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, FrameAllocator)

  ON_CORE_STARTUP
  {
    ezFrameAllocator::Startup();
  }

  ON_CORE_SHUTDOWN
  {
    ezFrameAllocator::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION

ezDoubleBufferedStackAllocator* ezFrameAllocator::s_pAllocator;

// static
void ezFrameAllocator::Swap()
{
  EZ_PROFILE("FrameAllocator.Swap");

  s_pAllocator->Swap();
}

// static
void ezFrameAllocator::Reset()
{
  s_pAllocator->Reset();
}

// static
void ezFrameAllocator::Startup()
{
  s_pAllocator = EZ_DEFAULT_NEW(ezDoubleBufferedStackAllocator, ezFoundation::GetAlignedAllocator());
}

// static
void ezFrameAllocator::Shutdown()
{
  EZ_DEFAULT_DELETE(s_pAllocator);
}

EZ_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_FrameAllocator);

