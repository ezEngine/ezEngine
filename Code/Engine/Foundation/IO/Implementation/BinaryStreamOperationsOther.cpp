#include <Foundation/PCH.h>
#include <Foundation/IO/IBinaryStream.h>
#include <Foundation/Time/Time.h>

void operator<< (ezIBinaryStreamWriter& Stream, const ezIAllocator::Stats& rhs)
{
  Stream << rhs.m_uiAllocationSize;
  Stream << rhs.m_uiNumAllocations;
  Stream << rhs.m_uiNumDeallocations;
  Stream << rhs.m_uiNumLiveAllocations;
  Stream << rhs.m_uiUsedMemorySize;
}

void operator>> (ezIBinaryStreamReader& Stream, ezIAllocator::Stats& rhs)
{
  Stream >> rhs.m_uiAllocationSize;
  Stream >> rhs.m_uiNumAllocations;
  Stream >> rhs.m_uiNumDeallocations;
  Stream >> rhs.m_uiNumLiveAllocations;
  Stream >> rhs.m_uiUsedMemorySize;
}

void operator<< (ezIBinaryStreamWriter& Stream, ezTime Value)
{
  Stream << Value.GetSeconds();
}

void operator>> (ezIBinaryStreamReader& Stream, ezTime& Value)
{
  double d = 0;
  Stream.ReadQWordValue(&d);

  Value = ezTime::Seconds(d);
}