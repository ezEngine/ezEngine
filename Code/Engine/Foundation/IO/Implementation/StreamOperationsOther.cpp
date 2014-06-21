#include <Foundation/PCH.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Time/Time.h>

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

EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_BinaryStreamOperationsOther);

