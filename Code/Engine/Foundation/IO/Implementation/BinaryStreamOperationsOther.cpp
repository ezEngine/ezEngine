#include <Foundation/PCH.h>
#include <Foundation/IO/IBinaryStream.h>

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