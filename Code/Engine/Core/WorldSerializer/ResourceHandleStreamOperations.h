#pragma once

#include <Core/CoreDLL.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezResource;
class ezStreamWriter;
class ezStreamReader;

class EZ_CORE_DLL ezResourceHandleStreamOperations
{
public:
  template <typename ResourceType>
  static void WriteHandle(ezStreamWriter& Stream, const ezTypedResourceHandle<ResourceType>& hResource)
  {
    WriteHandle(Stream, hResource.m_Typeless.m_pResource);
  }

  template <typename ResourceType>
  static void ReadHandle(ezStreamReader& Stream, ezTypedResourceHandle<ResourceType>& ResourceHandle)
  {
    ReadHandle(Stream, ResourceHandle.m_Typeless);
  }

private:
  static void WriteHandle(ezStreamWriter& Stream, const ezResource* pResource);
  static void ReadHandle(ezStreamReader& Stream, ezTypelessResourceHandle& ResourceHandle);
};

/// \brief Operator to serialize resource handles
template <typename ResourceType>
void operator<<(ezStreamWriter& Stream, const ezTypedResourceHandle<ResourceType>& Value)
{
  ezResourceHandleStreamOperations::WriteHandle(Stream, Value);
}

/// \brief Operator to deserialize resource handles
template <typename ResourceType>
void operator>>(ezStreamReader& Stream, ezTypedResourceHandle<ResourceType>& Value)
{
  ezResourceHandleStreamOperations::ReadHandle(Stream, Value);
}
