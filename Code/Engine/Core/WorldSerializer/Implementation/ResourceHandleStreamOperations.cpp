#include <CorePCH.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/ResourceHandleStreamOperations.h>

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

// static
void ezResourceHandleStreamOperations::ReadHandle(ezStreamReader& Stream, ezTypelessResourceHandle& ResourceHandle)
{
  ezStringBuilder sTemp;

  Stream >> sTemp;
  if (sTemp.IsEmpty())
  {
    ResourceHandle.Invalidate();
    return;
  }

  const ezRTTI* pRtti = ezRTTI::FindTypeByName(sTemp);
  if (pRtti == nullptr)
  {
    ezLog::Error("Unknown resource type '{0}'", sTemp);
    ResourceHandle.Invalidate();
  }

  // read unique ID for restoring the resource (from file)
  Stream >> sTemp;

  if (pRtti != nullptr)
  {
    ResourceHandle = ezResourceManager::LoadResourceByType(pRtti, sTemp);
  }
}


EZ_STATICLINK_FILE(Core, Core_WorldSerializer_Implementation_ResourceHandleStreamOperations);
