#include <TypeScriptPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <TypeScriptPlugin/Resources/JavaScriptResource.h>

// clang-format off
EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezJavaScriptResource);

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJavaScriptResource, 1, ezRTTIDefaultAllocator<ezJavaScriptResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezJavaScriptResource::ezJavaScriptResource()
  : ezResource(ezResource::DoUpdate::OnAnyThread, 1)
{
}

ezJavaScriptResource::~ezJavaScriptResource() = default;

ezResourceLoadDesc ezJavaScriptResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc ld;
  ld.m_State = ezResourceState::Unloaded;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;

  m_Desc.m_JsSource.Clear();
  m_Desc.m_JsSource.Compact();

  return ld;
}

ezResourceLoadDesc ezJavaScriptResource::UpdateContent(ezStreamReader* pStream)
{
  ezResourceLoadDesc ld;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;

  if (pStream == nullptr)
  {
    ld.m_State = ezResourceState::LoadedResourceMissing;
    return ld;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezString sAbsFilePath;
    (*pStream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*pStream);

  m_Desc.Deserialize(*pStream);

  ld.m_State = ezResourceState::Loaded;

  return ld;
}

void ezJavaScriptResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = (ezUInt32)sizeof(ezJavaScriptResource) + (ezUInt32)m_Desc.m_JsSource.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

//////////////////////////////////////////////////////////////////////////

ezResult ezJavaScriptResourceDesc::Serialize(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(stream.WriteArray(m_JsSource));
  EZ_SUCCEED_OR_RETURN(stream.WriteString(m_sComponentName));

  return EZ_SUCCESS;
}

ezResult ezJavaScriptResourceDesc::Deserialize(ezStreamReader& stream)
{
  /*ezTypeVersion version =*/stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(stream.ReadArray(m_JsSource));
  EZ_SUCCEED_OR_RETURN(stream.ReadString(m_sComponentName));

  return EZ_SUCCESS;
}
