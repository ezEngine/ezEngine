#include "Type.h"
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

ezTypeRTTI::ezTypeRTTI(const char* szTypeName, const ezTypeRTTI* pParentType)
{
  // TODO: we could parse away any namespace name

  m_szTypeName = szTypeName;
  
  // actually we don't need this ID at startup
  // we could just assign the IDs at a later point and then make sure they are sorted in a certain way
  // to speed up 'isDerivedFrom' checks, etc.
  m_iTypeID = GetRTTIClassID(szTypeName);

  m_pParentType = pParentType;
}

ezInt32 ezTypeRTTI::GetRTTIClassID(const char* szClassName)
{
  // This will be initialized at first call, so it is independent of static initializer order

  static ezMap<ezHybridString<32, ezStaticAllocatorWrapper>, ezInt32, ezCompareHelper<ezHybridString<32, ezStaticAllocatorWrapper>>, ezStaticAllocatorWrapper> ClassIDs;
  static ezInt32 iNextID = 0;

  ezMap<ezHybridString<32, ezStaticAllocatorWrapper>, ezInt32, ezCompareHelper<ezHybridString<32, ezStaticAllocatorWrapper>>,ezStaticAllocatorWrapper>::Iterator it = ClassIDs.Find (szClassName);

  if (it.IsValid())
    return it.Value();

  ++iNextID;
  ClassIDs[szClassName] = iNextID;

  return iNextID;
}


