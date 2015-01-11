#include <Foundation/PCH.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>

// ezAllocatorBase::Stats

void operator<< (ezStreamWriterBase& Stream, const ezAllocatorBase::Stats& rhs)
{
  Stream << rhs.m_uiNumAllocations;
  Stream << rhs.m_uiNumDeallocations;
  Stream << rhs.m_uiAllocationSize;
}

void operator>> (ezStreamReaderBase& Stream, ezAllocatorBase::Stats& rhs)
{
  Stream >> rhs.m_uiNumAllocations;
  Stream >> rhs.m_uiNumDeallocations;
  Stream >> rhs.m_uiAllocationSize;
}

// ezTime

void operator<< (ezStreamWriterBase& Stream, ezTime Value)
{
  Stream << Value.GetSeconds();
}

void operator>> (ezStreamReaderBase& Stream, ezTime& Value)
{
  double d = 0;
  Stream.ReadQWordValue(&d);

  Value = ezTime::Seconds(d);
}

// ezUuid

void operator<< (ezStreamWriterBase& Stream, const ezUuid& Value)
{
  Stream << Value.m_uiHigh;
  Stream << Value.m_uiLow;
}

void operator>> (ezStreamReaderBase& Stream, ezUuid& Value)
{
  Stream >> Value.m_uiHigh;
  Stream >> Value.m_uiLow;
}

EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StreamOperationsOther);

