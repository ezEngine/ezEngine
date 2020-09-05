#include <GameEnginePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <GameEngine/Curves/ColorGradientResource.h>

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
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  m_Descriptor.m_Gradient.Clear();

  return res;
}

ezResourceLoadDesc ezColorGradientResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezColorGradientResource::UpdateContent", GetResourceDescription().GetData());

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
  AssetHash.Read(*Stream);

  m_Descriptor.Load(*Stream);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezColorGradientResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU =
    static_cast<ezUInt32>(m_Descriptor.m_Gradient.GetHeapMemoryUsage()) + static_cast<ezUInt32>(sizeof(m_Descriptor));
}

void ezColorGradientResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;

  stream << uiVersion;

  m_Gradient.Save(stream);
}

void ezColorGradientResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;

  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion == 1, "Invalid file version {0}", uiVersion);

  m_Gradient.Load(stream);
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Curves_Implementation_ColorGradientResource);
