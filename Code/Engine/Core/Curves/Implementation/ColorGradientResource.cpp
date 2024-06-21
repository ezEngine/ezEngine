#include <Core/CorePCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezColorGradientResource, 1, ezRTTIDefaultAllocator<ezColorGradientResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezColorGradientResource);

ezColorGradientResource::ezColorGradientResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezColorGradientResource, ezColorGradientResourceDescriptor)
{
  m_Descriptor = descriptor;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

ezResourceLoadDesc ezColorGradientResource::UnloadData(Unload WhatToUnload)
{
  EZ_IGNORE_UNUSED(WhatToUnload);

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  m_Descriptor.m_Gradient.Clear();

  return res;
}

ezResourceLoadDesc ezColorGradientResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezColorGradientResource::UpdateContent", GetResourceIdOrDescription());

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    ezStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_Descriptor.Load(*Stream);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezColorGradientResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = static_cast<ezUInt32>(m_Descriptor.m_Gradient.GetHeapMemoryUsage()) + static_cast<ezUInt32>(sizeof(m_Descriptor));
}

void ezColorGradientResourceDescriptor::Save(ezStreamWriter& inout_stream) const
{
  const ezUInt8 uiVersion = 1;

  inout_stream << uiVersion;

  m_Gradient.Save(inout_stream);
}

void ezColorGradientResourceDescriptor::Load(ezStreamReader& inout_stream)
{
  ezUInt8 uiVersion = 0;

  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion == 1, "Invalid file version {0}", uiVersion);

  m_Gradient.Load(inout_stream);
}



EZ_STATICLINK_FILE(Core, Core_Curves_Implementation_ColorGradientResource);
