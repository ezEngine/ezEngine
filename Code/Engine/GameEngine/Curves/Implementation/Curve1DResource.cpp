#include <PCH.h>
#include <GameEngine/Curves/Curve1DResource.h>
#include <Core/Assets/AssetFileHeader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCurve1DResource, 1, ezRTTIDefaultAllocator<ezCurve1DResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE


ezCurve1DResource::ezCurve1DResource()
  : ezResource<ezCurve1DResource, ezCurve1DResourceDescriptor>(DoUpdate::OnAnyThread, 1)
{
}

ezResourceLoadDesc ezCurve1DResource::CreateResource(const ezCurve1DResourceDescriptor& descriptor)
{
  m_Descriptor = descriptor;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

ezResourceLoadDesc ezCurve1DResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 1;
  res.m_State = ezResourceState::Unloaded;

  m_Descriptor.m_Curves.Clear();

  return res;
}

ezResourceLoadDesc ezCurve1DResource::UpdateContent(ezStreamReader* Stream)
{
  EZ_LOG_BLOCK("ezCurve1DResource::UpdateContent", GetResourceDescription().GetData());

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
    ezString sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream);

  m_Descriptor.Load(*Stream);

  res.m_State = ezResourceState::Loaded;
  return res;
}

void ezCurve1DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = static_cast<ezUInt32>(m_Descriptor.m_Curves.GetHeapMemoryUsage()) + static_cast<ezUInt32>(sizeof(m_Descriptor));

  for (const auto& curve : m_Descriptor.m_Curves)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += static_cast<ezUInt32>(curve.GetHeapMemoryUsage());
  }
}

void ezCurve1DResourceDescriptor::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;

  stream << uiVersion;

  const ezUInt8 uiCurves = m_Curves.GetCount();
  stream << uiCurves;

  for (ezUInt32 i = 0; i < uiCurves; ++i)
  {
    m_Curves[i].Save(stream);
  }
}

void ezCurve1DResourceDescriptor::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;

  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion == 1, "Invalid file version {0}", uiVersion);

  ezUInt8 uiCurves = 0;
  stream >> uiCurves;

  m_Curves.SetCount(uiCurves);

  for (ezUInt32 i = 0; i < uiCurves; ++i)
  {
    m_Curves[i].Load(stream);

    /// \todo We can do this on load, or somehow ensure this is always already correctly saved
    m_Curves[i].SortControlPoints();
    m_Curves[i].CreateLinearApproximation();
  }
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_Curves_Implementation_Curve1DResource);

