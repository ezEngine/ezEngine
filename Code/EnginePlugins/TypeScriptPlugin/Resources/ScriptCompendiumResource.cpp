#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <TypeScriptPlugin/Resources/ScriptCompendiumResource.h>

// clang-format off
EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezScriptCompendiumResource);

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezScriptCompendiumResource, 1, ezRTTIDefaultAllocator<ezScriptCompendiumResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezScriptCompendiumResource::ezScriptCompendiumResource()
  : ezResource(ezResource::DoUpdate::OnAnyThread, 1)
{
}

ezScriptCompendiumResource::~ezScriptCompendiumResource() = default;

ezResourceLoadDesc ezScriptCompendiumResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc ld;
  ld.m_State = ezResourceState::Unloaded;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable = 0;

  m_Desc.m_PathToSource.Clear();

  return ld;
}

ezResourceLoadDesc ezScriptCompendiumResource::UpdateContent(ezStreamReader* pStream)
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
  AssetHash.Read(*pStream).IgnoreResult();

  m_Desc.Deserialize(*pStream).IgnoreResult();

  ld.m_State = ezResourceState::Loaded;

  return ld;
}

void ezScriptCompendiumResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = (ezUInt32)sizeof(ezScriptCompendiumResource) + (ezUInt32)m_Desc.m_PathToSource.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

//////////////////////////////////////////////////////////////////////////

ezResult ezScriptCompendiumResourceDesc::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(2);

  EZ_SUCCEED_OR_RETURN(inout_stream.WriteMap(m_PathToSource));
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteMap(m_AssetGuidToInfo));

  return EZ_SUCCESS;
}

ezResult ezScriptCompendiumResourceDesc::Deserialize(ezStreamReader& inout_stream)
{
  ezTypeVersion version = inout_stream.ReadVersion(2);

  EZ_SUCCEED_OR_RETURN(inout_stream.ReadMap(m_PathToSource));

  if (version >= 2)
  {
    EZ_SUCCEED_OR_RETURN(inout_stream.ReadMap(m_AssetGuidToInfo));
  }

  return EZ_SUCCESS;
}

ezResult ezScriptCompendiumResourceDesc::ComponentTypeInfo::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(inout_stream.WriteString(m_sComponentTypeName));
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteString(m_sComponentFilePath));
  return EZ_SUCCESS;
}

ezResult ezScriptCompendiumResourceDesc::ComponentTypeInfo::Deserialize(ezStreamReader& inout_stream)
{
  ezTypeVersion version = inout_stream.ReadVersion(1);
  EZ_IGNORE_UNUSED(version);

  EZ_SUCCEED_OR_RETURN(inout_stream.ReadString(m_sComponentTypeName));
  EZ_SUCCEED_OR_RETURN(inout_stream.ReadString(m_sComponentFilePath));
  return EZ_SUCCESS;
}
