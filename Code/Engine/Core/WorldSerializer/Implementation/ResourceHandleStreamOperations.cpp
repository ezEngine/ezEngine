#include <CorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/ResourceHandleStreamOperations.h>

// ReadHandle Implementation is currently in ResourceHandleReader.cpp

// static
void ezResourceHandleStreamOperations::WriteHandle(ezStreamWriter& Stream, const ezResource* pResource)
{
  if (pResource != nullptr)
  {
    Stream << pResource->GetDynamicRTTI()->GetTypeName();
    Stream << pResource->GetResourceID();
  }
  else
  {
    const char* szEmpty = "";
    Stream << szEmpty;
  }
}


EZ_STATICLINK_FILE(Core, Core_WorldSerializer_Implementation_ResourceHandleStreamOperations);

