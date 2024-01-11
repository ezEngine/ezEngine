#include <Core/CorePCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Core/Assets/AssetFileHeader.h>
#include <Core/Utils/CustomData.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Serialization/BinarySerializer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCustomData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezCustomData::Load(ezAbstractObjectGraph& ref_graph, ezRttiConverterContext& ref_context, const ezAbstractObjectNode* pRootNode)
{
  ezRttiConverterReader convRead(&ref_graph, &ref_context);
  convRead.ApplyPropertiesToObject(pRootNode, GetDynamicRTTI(), this);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCustomDataResourceBase, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezCustomDataResourceBase::ezCustomDataResourceBase() : ezResource(DoUpdate::OnAnyThread, 1)
{
}

ezCustomDataResourceBase::~ezCustomDataResourceBase() = default;

ezResourceLoadDesc ezCustomDataResourceBase::UnloadData(Unload WhatToUnload)
{
  ezResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = ezResourceState::Unloaded;
  return res;
}

ezResourceLoadDesc ezCustomDataResourceBase::UpdateContent_Internal(ezStreamReader* Stream, const ezRTTI& rtti)
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
  AssetHash.Read(*Stream).IgnoreResult();

  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;
  const ezAbstractObjectNode* pRootNode;

  ezAbstractGraphBinarySerializer::Read(*Stream, &graph);

  pRootNode = graph.GetNodeByName("root");
  if (pRootNode == nullptr || pRootNode->GetType() != rtti.GetTypeName())
  {
    ezLog::Error("Expecting custom data of type '{0}' but given '{1}'", rtti.GetTypeName(), pRootNode->GetType());
    res.m_State = ezResourceState::LoadedResourceMissing;
    return res;
  }

  CreateAndLoadData(graph, context, pRootNode);

  res.m_State = ezResourceState::Loaded;
  return res;
}

EZ_STATICLINK_FILE(Core, Core_Utils_Implementation_CustomData);
