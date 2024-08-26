#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezRmlUiScaleMode, 1)
  EZ_ENUM_CONSTANTS(ezRmlUiScaleMode::Fixed, ezRmlUiScaleMode::WithScreenSize)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

static ezTypeVersion s_RmlUiDescVersion = 1;

ezResult ezRmlUiResourceDescriptor::Save(ezStreamWriter& inout_stream)
{
  // write this at the beginning so that the file can be read as an ezDependencyFile
  m_DependencyFile.StoreCurrentTimeStamp();
  EZ_SUCCEED_OR_RETURN(m_DependencyFile.WriteDependencyFile(inout_stream));

  inout_stream.WriteVersion(s_RmlUiDescVersion);

  inout_stream << m_sRmlFile;
  inout_stream << m_ScaleMode;
  inout_stream << m_ReferenceResolution;

  return EZ_SUCCESS;
}

ezResult ezRmlUiResourceDescriptor::Load(ezStreamReader& inout_stream)
{
  EZ_SUCCEED_OR_RETURN(m_DependencyFile.ReadDependencyFile(inout_stream));

  ezTypeVersion uiVersion = inout_stream.ReadVersion(s_RmlUiDescVersion);
  EZ_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_sRmlFile;
  inout_stream >> m_ScaleMode;
  inout_stream >> m_ReferenceResolution;

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiResource, 1, ezRTTIDefaultAllocator<ezRmlUiResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezRmlUiResource);
// clang-format on

ezRmlUiResource::ezRmlUiResource()
  : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezResourceLoadDesc ezRmlUiResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;

  return res;
}

ezResourceLoadDesc ezRmlUiResource::UpdateContent(ezStreamReader* Stream)
{
  ezRmlUiResourceDescriptor desc;
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  ezStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // Direct loading of rml file
  if (sAbsFilePath.GetFileExtension() == "rml")
  {
    m_sRmlFile = sAbsFilePath;

    res.m_State = ezResourceState::Loaded;
    return res;
  }

  ezAssetFileHeader assetHeader;
  assetHeader.Read(*Stream).IgnoreResult();

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(desc));
}

void ezRmlUiResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

EZ_RESOURCE_IMPLEMENT_CREATEABLE(ezRmlUiResource, ezRmlUiResourceDescriptor)
{
  m_sRmlFile = descriptor.m_sRmlFile;
  m_ScaleMode = descriptor.m_ScaleMode;
  m_vReferenceResolution = descriptor.m_ReferenceResolution;

  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Loaded;

  return res;
}

//////////////////////////////////////////////////////////////////////////

bool ezRmlUiResourceLoader::IsResourceOutdated(const ezResource* pResource) const
{
  if (ezResourceLoaderFromFile::IsResourceOutdated(pResource))
    return true;

  ezStringBuilder sId = pResource->GetResourceID();
  if (sId.GetFileExtension() == "rml")
    return false;

  ezFileReader stream;
  if (stream.Open(pResource->GetResourceID()).Failed())
    return false;

  // skip asset header
  ezAssetFileHeader assetHeader;
  assetHeader.Read(stream).IgnoreResult();

  ezDependencyFile dep;
  if (dep.ReadDependencyFile(stream).Failed())
    return true;

  return dep.HasAnyFileChanged();
}
