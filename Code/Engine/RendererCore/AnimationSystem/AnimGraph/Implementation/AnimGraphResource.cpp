#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationClipMapping, 1, ezRTTIDefaultAllocator<ezAnimationClipMapping>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("ClipName", GetClipName, SetClipName)->AddAttributes(new ezDynamicStringEnumAttribute("AnimationClipMappingEnum")),
    EZ_RESOURCE_MEMBER_PROPERTY("Clip", m_hClip)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
  }
    EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphResource, 1, ezRTTIDefaultAllocator<ezAnimGraphResource>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_RESOURCE_IMPLEMENT_COMMON_CODE(ezAnimGraphResource);
// clang-format on

ezAnimGraphResource::ezAnimGraphResource()
  : ezResource(ezResource::DoUpdate::OnAnyThread, 0)
{
}

ezAnimGraphResource::~ezAnimGraphResource() = default;

ezResourceLoadDesc ezAnimGraphResource::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc d;
  d.m_State = ezResourceState::Unloaded;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable = 0;
  return d;
}

ezResourceLoadDesc ezAnimGraphResource::UpdateContent(ezStreamReader* Stream)
{
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

  ezAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).AssertSuccess();

  {
    const auto uiVersion = Stream->ReadVersion(2);
    Stream->ReadArray(m_IncludeGraphs).AssertSuccess();

    if (uiVersion >= 2)
    {
      ezUInt32 uiNum = 0;
      *Stream >> uiNum;

      m_AnimationClipMapping.SetCount(uiNum);
      for (ezUInt32 i = 0; i < uiNum; ++i)
      {
        *Stream >> m_AnimationClipMapping[i].m_sClipName;
        *Stream >> m_AnimationClipMapping[i].m_hClip;
      }
    }
  }

  if (m_AnimGraph.Deserialize(*Stream).Failed())
  {
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  m_AnimGraph.PrepareForUse();

  res.m_State = ezResourceState::Loaded;

  return res;
}

void ezAnimGraphResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = 0;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Implementation_AnimGraphResource);
